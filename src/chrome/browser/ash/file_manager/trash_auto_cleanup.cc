// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ash/file_manager/trash_auto_cleanup.h"

#include "base/barrier_callback.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_functions.h"
#include "base/system/sys_info.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "chrome/browser/ash/file_manager/trash_common_util.h"

namespace file_manager::trash {

namespace {

constexpr char kCleanupFileCountMetricName[] =
    "FileBrowser.TrashAutoCleanup.FileCount";
constexpr char kCleanupErrorsMetricName[] =
    "FileBrowser.TrashAutoCleanup.Errors";
constexpr char kCleanupTimeMetricName[] = "FileBrowser.TrashAutoCleanup.Time";

// Enumerates Trash info files (supported .Trash/info/ locations) and returns
// the list of trashinfo files corresponding to the trash entries to delete.
// A maximum of `kMaxBatchSize` trashinfo files is returned.
std::vector<base::FilePath> GetTrashInfoFilesToDeleteOnBlockingThread(
    const std::vector<base::FilePath>& trash_info_directories) {
  std::vector<base::FilePath> trash_info_paths_to_delete;
  base::Time now = base::Time::Now();
  int invalid_file_counter = 0;
  int file_get_info_failed_counter = 0;
  for (const base::FilePath& dir : trash_info_directories) {
    base::FileEnumerator file_iter(dir, false, base::FileEnumerator::FILES);
    while (!file_iter.Next().empty()) {
      const std::string file_name = file_iter.GetInfo().GetName().value();
      const base::FilePath trash_info_path = dir.Append(file_name);
      // Get last modified time.
      base::File file(trash_info_path,
                      base::File::FLAG_OPEN | base::File::FLAG_READ);
      if (!file.IsValid()) {
        ++invalid_file_counter;
        base::UmaHistogramEnumeration(kCleanupErrorsMetricName,
                                      AutoCleanupError::kInvalidTrashInfoFile);
        continue;
      }
      base::File::Info info;
      if (!file.GetInfo(&info)) {
        ++file_get_info_failed_counter;
        base::UmaHistogramEnumeration(
            kCleanupErrorsMetricName,
            AutoCleanupError::kFailedToGetTrashInfoFileModifiedTime);
        continue;
      }
      if (now - info.last_modified >= kMaxTrashAge) {
        trash_info_paths_to_delete.push_back(trash_info_path);
        if (trash_info_paths_to_delete.size() >= kMaxBatchSize) {
          break;
        }
      }
    }
    if (trash_info_paths_to_delete.size() >= kMaxBatchSize) {
      break;
    }
  }
  if (invalid_file_counter) {
    LOG(ERROR) << invalid_file_counter << " invalid trashinfo files";
  }
  if (file_get_info_failed_counter) {
    LOG(ERROR) << "Could not get info from " << file_get_info_failed_counter
               << " trashinfo files";
  }
  base::UmaHistogramCounts1000(kCleanupFileCountMetricName,
                               trash_info_paths_to_delete.size());
  return trash_info_paths_to_delete;
}

bool DeleteOldTrashFilesOnBlockingThread(
    std::vector<ParsedTrashInfoData> to_delete) {
  bool success = true;
  for (const ParsedTrashInfoData& trash_info_data : to_delete) {
    if (!base::DeleteFile(trash_info_data.trashed_file_path) ||
        !base::DeleteFile(trash_info_data.trash_info_path)) {
      base::UmaHistogramEnumeration(kCleanupErrorsMetricName,
                                    AutoCleanupError::kFailedToDeleteTrashFile);
      success = false;
    } else {
      base::UmaHistogramEnumeration(kCleanupErrorsMetricName,
                                    AutoCleanupError::kSuccessfullyDeleted);
    }
  }
  return success;
}

}  // namespace

TrashAutoCleanup::TrashAutoCleanup(Profile* profile) : profile_(profile) {
  const TrashPathsMap trash_locations_ =
      file_manager::trash::GenerateEnabledTrashLocationsForProfile(profile_);
  for (const trash::TrashPathsMap::value_type& location : trash_locations_) {
    trash_info_directories_.push_back(
        location.first.Append(location.second.relative_folder_path)
            .Append(kInfoFolderName));
  }
}

TrashAutoCleanup::~TrashAutoCleanup() = default;

std::unique_ptr<TrashAutoCleanup> TrashAutoCleanup::Create(Profile* profile) {
  // Only run the auto cleanup process for regular profiles on ChromeOS.
  if (!file_manager::trash::IsTrashEnabledForProfile(profile) || !profile ||
      !profile->IsRegularProfile() || !base::SysInfo::IsRunningOnChromeOS()) {
    return nullptr;
  }

  auto instance = base::WrapUnique(new TrashAutoCleanup(profile));
  instance->Init();
  return instance;
}

void TrashAutoCleanup::Init() {
  cleanup_repeating_timer_.Start(
      FROM_HERE, kCleanupCheckInterval,
      base::BindRepeating(&TrashAutoCleanup::StartCleanup,
                          weak_ptr_factory_.GetWeakPtr()));
}

void TrashAutoCleanup::StartCleanup() {
  // "TrashEnabled" can be dynamically refreshed, make sure that it's enabled.
  if (!file_manager::trash::IsTrashEnabledForProfile(profile_)) {
    return;
  }

  if (!last_cleanup_time_.is_null() &&
      base::Time::Now() - last_cleanup_time_ < kCleanupInterval) {
    // Skip cleanup iteration if it last happened less than a day earlier.
    if (cleanup_done_closure_for_test_) {
      std::move(cleanup_done_closure_for_test_)
          .Run(AutoCleanupResult::kWaitingForNextCleanupIteration);
    }
    return;
  }
  last_cleanup_time_ = base::Time::Now();
  cleanup_start_time_ = base::TimeTicks::Now();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
       base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&GetTrashInfoFilesToDeleteOnBlockingThread,
                     trash_info_directories_),
      base::BindOnce(&TrashAutoCleanup::OnTrashInfoFilesToDeleteEnumerated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TrashAutoCleanup::OnTrashInfoFilesToDeleteEnumerated(
    const std::vector<base::FilePath>& trash_info_paths_to_delete) {
  if (trash_info_paths_to_delete.empty()) {
    if (cleanup_done_closure_for_test_) {
      std::move(cleanup_done_closure_for_test_)
          .Run(AutoCleanupResult::kNoOldFilesToCleanup);
    }
    return;
  }
  if (trash_info_paths_to_delete.size() == kMaxBatchSize) {
    // The maximum batch size has been reached: there is more likely going to be
    // more files to cleanup. Unset the last cleanup time to force the next
    // iteration.
    last_cleanup_time_ = base::Time();
  }
  validator_ =
      std::make_unique<file_manager::trash::TrashInfoValidator>(profile_);
  auto barrier_callback =
      base::BarrierCallback<file_manager::trash::ParsedTrashInfoDataOrError>(
          trash_info_paths_to_delete.size(),
          base::BindOnce(&TrashAutoCleanup::OnTrashInfoFilesParsed,
                         weak_ptr_factory_.GetWeakPtr()));
  for (const base::FilePath& path : trash_info_paths_to_delete) {
    validator_->ValidateAndParseTrashInfo(std::move(path), barrier_callback);
  }
}

void TrashAutoCleanup::OnTrashInfoFilesParsed(
    std::vector<ParsedTrashInfoDataOrError> parsed_data_or_error) {
  validator_.reset();
  std::vector<ParsedTrashInfoData> to_delete;
  int parse_error_counter = 0;
  for (auto& trash_info_data_or_error : parsed_data_or_error) {
    if (!trash_info_data_or_error.has_value()) {
      ++parse_error_counter;
      base::UmaHistogramEnumeration(
          kCleanupErrorsMetricName,
          AutoCleanupError::kFailedToParseTrashInfoFile);
      continue;
    }
    to_delete.push_back(std::move(trash_info_data_or_error.value()));
  }
  if (parse_error_counter) {
    LOG(ERROR) << "Failed to parse " << parse_error_counter
               << " trash info files";
    if (cleanup_done_closure_for_test_) {
      std::move(cleanup_done_closure_for_test_)
          .Run(AutoCleanupResult::kTrashInfoParsingError);
    }
  }
  if (to_delete.empty()) {
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN,
       base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&DeleteOldTrashFilesOnBlockingThread,
                     std::move(to_delete)),
      base::BindOnce(&TrashAutoCleanup::OnCleanupDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TrashAutoCleanup::OnCleanupDone(bool success) {
  if (cleanup_done_closure_for_test_) {
    const AutoCleanupResult result = success
                                         ? AutoCleanupResult::kCleanupSuccessful
                                         : AutoCleanupResult::kDeletionError;
    std::move(cleanup_done_closure_for_test_).Run(result);
  }
  base::UmaHistogramTimes(kCleanupTimeMetricName,
                          base::TimeTicks::Now() - cleanup_start_time_);
}

void TrashAutoCleanup::SetCleanupDoneCallbackForTest(
    base::OnceCallback<void(AutoCleanupResult result)> cleanup_done_closure) {
  cleanup_done_closure_for_test_ = std::move(cleanup_done_closure);
}

}  // namespace file_manager::trash

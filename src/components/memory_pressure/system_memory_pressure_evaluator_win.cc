// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/memory_pressure/system_memory_pressure_evaluator_win.h"

#include <windows.h>

#include <psapi.h>

#include <algorithm>
#include <memory>

#include "base/functional/bind.h"
#include "base/metrics/histogram_functions.h"
#include "base/system/sys_info.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/win/object_watcher.h"
#include "components/memory_pressure/multi_source_memory_pressure_monitor.h"

namespace memory_pressure {
namespace win {

namespace {

static const DWORDLONG kMBBytes = 1024 * 1024;

// Implements ObjectWatcher::Delegate by forwarding to a provided callback.
class MemoryPressureWatcherDelegate
    : public base::win::ObjectWatcher::Delegate {
 public:
  MemoryPressureWatcherDelegate(base::win::ScopedHandle handle,
                                base::OnceClosure callback);
  ~MemoryPressureWatcherDelegate() override;
  MemoryPressureWatcherDelegate(const MemoryPressureWatcherDelegate& other) =
      delete;
  MemoryPressureWatcherDelegate& operator=(
      const MemoryPressureWatcherDelegate&) = delete;

  void ReplaceWatchedHandleForTesting(base::win::ScopedHandle handle);
  void SetCallbackForTesting(base::OnceClosure callback) {
    callback_ = std::move(callback);
  }

 private:
  void OnObjectSignaled(HANDLE handle) override;

  base::win::ScopedHandle handle_;
  base::win::ObjectWatcher watcher_;
  base::OnceClosure callback_;
};

MemoryPressureWatcherDelegate::MemoryPressureWatcherDelegate(
    base::win::ScopedHandle handle,
    base::OnceClosure callback)
    : handle_(std::move(handle)), callback_(std::move(callback)) {
  DCHECK(handle_.IsValid());
  CHECK(watcher_.StartWatchingOnce(handle_.Get(), this));
}

MemoryPressureWatcherDelegate::~MemoryPressureWatcherDelegate() = default;

void MemoryPressureWatcherDelegate::ReplaceWatchedHandleForTesting(
    base::win::ScopedHandle handle) {
  if (watcher_.IsWatching()) {
    watcher_.StopWatching();
  }
  handle_ = std::move(handle);
  CHECK(watcher_.StartWatchingOnce(handle_.Get(), this));
}

void MemoryPressureWatcherDelegate::OnObjectSignaled(HANDLE handle) {
  DCHECK_EQ(handle, handle_.Get());
  std::move(callback_).Run();
}

}  // namespace

// Check the amount of RAM left every 5 seconds.
const base::TimeDelta SystemMemoryPressureEvaluator::kMemorySamplingPeriod =
    base::Seconds(5);

// The following constants have been lifted from similar values in the ChromeOS
// memory pressure monitor. The values were determined experimentally to ensure
// sufficient responsiveness of the memory pressure subsystem, and minimal
// overhead.
const base::TimeDelta SystemMemoryPressureEvaluator::kModeratePressureCooldown =
    base::Seconds(10);

// TODO(chrisha): Explore the following constants further with an experiment.

// A system is considered 'high memory' if it has more than 1.5GB of system
// memory available for use by the memory manager (not reserved for hardware
// and drivers). This is a fuzzy version of the ~2GB discussed below.
const int SystemMemoryPressureEvaluator::kLargeMemoryThresholdMb = 1536;

// These are the default thresholds used for systems with < ~2GB of physical
// memory. Such systems have been observed to always maintain ~100MB of
// available memory, paging until that is the case. To try to avoid paging a
// threshold slightly above this is chosen. The moderate threshold is slightly
// less grounded in reality and chosen as 2.5x critical.
const int
    SystemMemoryPressureEvaluator::kSmallMemoryDefaultModerateThresholdMb = 500;
const int
    SystemMemoryPressureEvaluator::kSmallMemoryDefaultCriticalThresholdMb = 200;

// These are the default thresholds used for systems with >= ~2GB of physical
// memory. Such systems have been observed to always maintain ~300MB of
// available memory, paging until that is the case.
const int
    SystemMemoryPressureEvaluator::kLargeMemoryDefaultModerateThresholdMb =
        1000;
const int
    SystemMemoryPressureEvaluator::kLargeMemoryDefaultCriticalThresholdMb = 400;

SystemMemoryPressureEvaluator::SystemMemoryPressureEvaluator(
    std::unique_ptr<MemoryPressureVoter> voter)
    : memory_pressure::SystemMemoryPressureEvaluator(std::move(voter)),
      moderate_threshold_mb_(0),
      critical_threshold_mb_(0),
      moderate_pressure_repeat_count_(0) {
  InferThresholds();
  StartObserving();
}

SystemMemoryPressureEvaluator::SystemMemoryPressureEvaluator(
    int moderate_threshold_mb,
    int critical_threshold_mb,
    std::unique_ptr<MemoryPressureVoter> voter)
    : memory_pressure::SystemMemoryPressureEvaluator(std::move(voter)),
      moderate_threshold_mb_(moderate_threshold_mb),
      critical_threshold_mb_(critical_threshold_mb),
      moderate_pressure_repeat_count_(0) {
  DCHECK_GE(moderate_threshold_mb_, critical_threshold_mb_);
  DCHECK_LE(0, critical_threshold_mb_);
  StartObserving();
}

SystemMemoryPressureEvaluator::~SystemMemoryPressureEvaluator() {
  StopObserving();
}

void SystemMemoryPressureEvaluator::InferThresholds() {
  // Default to a 'high' memory situation, which uses more conservative
  // thresholds.
  bool high_memory = true;
  MEMORYSTATUSEX mem_status = {};
  if (GetSystemMemoryStatus(&mem_status)) {
    static const DWORDLONG kLargeMemoryThresholdBytes =
        static_cast<DWORDLONG>(kLargeMemoryThresholdMb) * kMBBytes;
    high_memory = mem_status.ullTotalPhys >= kLargeMemoryThresholdBytes;
  }

  if (high_memory) {
    moderate_threshold_mb_ = kLargeMemoryDefaultModerateThresholdMb;
    critical_threshold_mb_ = kLargeMemoryDefaultCriticalThresholdMb;
  } else {
    moderate_threshold_mb_ = kSmallMemoryDefaultModerateThresholdMb;
    critical_threshold_mb_ = kSmallMemoryDefaultCriticalThresholdMb;
  }
}

void SystemMemoryPressureEvaluator::StartObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  timer_.Start(
      FROM_HERE, kMemorySamplingPeriod,
      BindRepeating(&SystemMemoryPressureEvaluator::CheckMemoryPressure,
                    weak_ptr_factory_.GetWeakPtr()));
}

void SystemMemoryPressureEvaluator::StopObserving() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If StartObserving failed, StopObserving will still get called.
  timer_.Stop();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void SystemMemoryPressureEvaluator::CheckMemoryPressure() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Get the previous pressure level and update the current one.
  MemoryPressureLevel old_vote = current_vote();
  SetCurrentVote(CalculateCurrentPressureLevel());

  // |notify| will be set to true if MemoryPressureListeners need to be
  // notified of a memory pressure level state change.
  bool notify = false;
  switch (current_vote()) {
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
      break;

    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
      if (old_vote != current_vote()) {
        // This is a new transition to moderate pressure so notify.
        moderate_pressure_repeat_count_ = 0;
        notify = true;
      } else {
        // Already in moderate pressure, only notify if sustained over the
        // cooldown period.
        const int kModeratePressureCooldownCycles =
            kModeratePressureCooldown / kMemorySamplingPeriod;
        if (++moderate_pressure_repeat_count_ ==
            kModeratePressureCooldownCycles) {
          moderate_pressure_repeat_count_ = 0;
          notify = true;
        }
      }
      break;

    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL:
      // Always notify of critical pressure levels.
      notify = true;
      break;
  }

  SendCurrentVote(notify);
}

base::MemoryPressureListener::MemoryPressureLevel
SystemMemoryPressureEvaluator::CalculateCurrentPressureLevel() {
  // First to try to collect commit limit histograms. Otherwise, there's a
  // chance of an early exit from this function (e.g. if
  // `GetSystemMemoryStatus` fails), which would cause us to miss collecting the
  // histogram data.
  RecordCommitHistograms();

  MEMORYSTATUSEX mem_status = {};
  if (!GetSystemMemoryStatus(&mem_status)) {
    return base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  }

  // How much system memory is actively available for use right now, in MBs.
  int phys_free = static_cast<int>(mem_status.ullAvailPhys / kMBBytes);

  // TODO(chrisha): This should eventually care about address space pressure,
  // but the browser process (where this is running) effectively never runs out
  // of address space. Renderers occasionally do, but it does them no good to
  // have the browser process monitor address space pressure. Long term,
  // renderers should run their own address space pressure monitors and act
  // accordingly, with the browser making cross-process decisions based on
  // system memory pressure.

  // Determine if the physical memory is under critical memory pressure.
  if (phys_free <= critical_threshold_mb_) {
    return base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  }

  // Determine if the physical memory is under moderate memory pressure.
  if (phys_free <= moderate_threshold_mb_) {
    return base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
  }

  // No memory pressure was detected.
  return base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
}

bool SystemMemoryPressureEvaluator::GetSystemMemoryStatus(
    MEMORYSTATUSEX* mem_status) {
  DCHECK(mem_status);
  mem_status->dwLength = sizeof(*mem_status);
  if (!::GlobalMemoryStatusEx(mem_status)) {
    return false;
  }
  return true;
}

void SystemMemoryPressureEvaluator::RecordCommitHistograms() {
  PERFORMANCE_INFORMATION performance_info = {};
  bool get_performance_info_result =
      GetPerformanceInfoWrapper(&performance_info);
  base::UmaHistogramBoolean("Memory.PerformanceInfoRetrievalSuccess",
                            get_performance_info_result);

  if (!get_performance_info_result) {
    return;
  }
  // Calculate commit limit in MB.
  uint64_t commit_limit_mb =
      base::strict_cast<uint64_t>(performance_info.CommitLimit *
                                  performance_info.PageSize) /
      kMBBytes;
  // Calculate amount of memory in MB currently committed by the system.
  uint64_t commit_total_mb =
      base::strict_cast<uint64_t>(performance_info.CommitTotal *
                                  performance_info.PageSize) /
      kMBBytes;

  // Calculate remaining commit limit in MB.
  uint64_t remaining_limit_mb = commit_limit_mb - commit_total_mb;

  int commit_limit_int = std::clamp(static_cast<int>(commit_limit_mb), 0,
                                    std::numeric_limits<int>::max());

  int remaining_limit_int = std::clamp(static_cast<int>(remaining_limit_mb), 0,
                                       std::numeric_limits<int>::max());

  base::UmaHistogramCounts10M("Memory.CommitLimitMB", commit_limit_int);
  base::UmaHistogramCounts10M("Memory.CommitRemainingMB", remaining_limit_int);

  // Calculate percentage used
  uint64_t temp_percentage = (commit_total_mb * 100) / commit_limit_mb;
  int percentage_used = base::saturated_cast<int>(temp_percentage);

  base::UmaHistogramPercentage("Memory.CommitPercentageUsed", percentage_used);
}

bool SystemMemoryPressureEvaluator::GetPerformanceInfoWrapper(
    PERFORMANCE_INFORMATION* perf_info) {
  DCHECK(perf_info);

  return GetPerformanceInfo(perf_info, sizeof(*perf_info));
}

}  // namespace win
}  // namespace memory_pressure

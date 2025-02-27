// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SURVEY_DESKTOP_CONTROLLER_H_
#define CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SURVEY_DESKTOP_CONTROLLER_H_

#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/hats/hats_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/privacy_sandbox/privacy_sandbox_survey_service.h"

#if BUILDFLAG(IS_ANDROID)
#error This file should only be included on desktop.
#endif

namespace privacy_sandbox {

// This controller is responsible for managing privacy sandbox surveys for
// desktop.
class PrivacySandboxSurveyDesktopController : public KeyedService {
 public:
  explicit PrivacySandboxSurveyDesktopController(
      PrivacySandboxSurveyService* survey_service);
  ~PrivacySandboxSurveyDesktopController() override;

  // Called to surface the sentiment survey if the conditions are met.
  void MaybeShowSentimentSurvey(Profile* profile);

  // Called to denote that we've visited a new tab page.
  void OnNewTabPageSeen();

 private:
  void OnSentimentSurveyShown(Profile* profile);
  void OnSentimentSurveyFailure();

  // Tracks if a NTP has been seen within the current session.
  bool has_seen_ntp_ = false;
  raw_ptr<PrivacySandboxSurveyService> survey_service_;
  base::WeakPtrFactory<PrivacySandboxSurveyDesktopController> weak_ptr_factory_{
      this};

  friend class PrivacySandboxSurveyDesktopControllerLaunchSurveyTest;
};

}  // namespace privacy_sandbox

#endif  // CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SURVEY_DESKTOP_CONTROLLER_H_

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/privacy_sandbox/privacy_sandbox_tab_observer.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "privacy_sandbox_survey_desktop_controller.h"
#include "privacy_sandbox_survey_desktop_controller_factory.h"
#include "privacy_sandbox_tab_observer.h"

namespace privacy_sandbox {

PrivacySandboxTabObserver::PrivacySandboxTabObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

PrivacySandboxTabObserver::~PrivacySandboxTabObserver() = default;

void PrivacySandboxTabObserver::PrimaryPageChanged(content::Page& page) {
  if (IsNewTabPage()) {
    Profile* profile =
        Profile::FromBrowserContext(web_contents()->GetBrowserContext());
    auto* desktop_survey_controller =
        PrivacySandboxSurveyDesktopControllerFactory::GetForProfile(profile);
    if (desktop_survey_controller) {
      desktop_survey_controller->MaybeShowSentimentSurvey(profile);
      desktop_survey_controller->OnNewTabPageSeen();
    }
  }
}

bool PrivacySandboxTabObserver::IsNewTabPage() {
  return web_contents()->GetLastCommittedURL() ==
             chrome::kChromeUINewTabPageURL ||
         web_contents()->GetLastCommittedURL() == chrome::kChromeUINewTabURL;
}

}  // namespace privacy_sandbox

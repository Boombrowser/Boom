// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_H_

#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/internal_webui_config.h"
#include "content/public/browser/web_ui_controller.h"

class UserActionsUI;

class UserActionsUIConfig
    : public content::DefaultInternalWebUIConfig<UserActionsUI> {
 public:
  UserActionsUIConfig()
      : DefaultInternalWebUIConfig(chrome::kChromeUIUserActionsHost) {}
};

// The UI for chrome://user-actions/
class UserActionsUI : public content::WebUIController {
 public:
  explicit UserActionsUI(content::WebUI* contents);

  UserActionsUI(const UserActionsUI&) = delete;
  UserActionsUI& operator=(const UserActionsUI&) = delete;

  ~UserActionsUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_USER_ACTIONS_USER_ACTIONS_UI_H_

// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/log_web_ui_url.h"

#include "chrome/common/webui_url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

TEST(LogWebUIUrlTest, ValidUrls) {
  // chrome:// WebUI page.
  EXPECT_TRUE(webui::LogWebUICreated(GURL(chrome::kChromeUIDownloadsURL)));

#if !BUILDFLAG(IS_ANDROID)
  // chrome-untrusted:// WebUI page.
  EXPECT_TRUE(webui::LogWebUICreated(GURL(chrome::kChromeUIUntrustedPrintURL)));
#endif

  // WebUI page with a subpage.
  GURL::Replacements replace_clear_data_path;
  replace_clear_data_path.SetPathStr(chrome::kClearBrowserDataSubPage);
  EXPECT_TRUE(
      webui::LogWebUICreated(GURL(chrome::kChromeUISettingsURL)
                                 .ReplaceComponents(replace_clear_data_path)));

  // Developer tools scheme.
  EXPECT_TRUE(webui::LogWebUICreated(GURL("devtools://devtools")));
}

TEST(LogWebUIUrlTest, InvalidUrls) {
  // HTTP/HTTPS/FTP/etc. schemes should be ignored.
  EXPECT_FALSE(webui::LogWebUICreated(GURL("http://google.com?q=pii")));
  EXPECT_FALSE(webui::LogWebUICreated(GURL("https://facebook.com")));
  EXPECT_FALSE(webui::LogWebUICreated(GURL("ftp://ftp.mysite.com")));

  // Extensions schemes should also be ignored.
  EXPECT_FALSE(webui::LogWebUICreated(
      GURL("chrome-extension://mfehgcgbbipciphmccgaenjidiccnmng")));
}

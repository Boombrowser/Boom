// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAGE_INFO_CORE_FEATURES_H_
#define COMPONENTS_PAGE_INFO_CORE_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"

namespace page_info {

// Enables the "About this site" section in Page Info.
extern bool IsAboutThisSiteFeatureEnabled(const std::string& locale);

// Controls the feature for English and other languages that are enabled by
// default. Use IsAboutThisSiteFeatureEnabled() to check a specific language.
BASE_DECLARE_FEATURE(kPageInfoAboutThisSite);
// Controls the feature for languages that are not enabled by default yet.
BASE_DECLARE_FEATURE(kPageInfoAboutThisSiteMoreLangs);

// Whether we show hard-coded content for some sites like https://example.com.
extern const base::FeatureParam<bool> kShowSampleContent;

#if !BUILDFLAG(IS_ANDROID)
// Enables the history section for Page Info on desktop.
BASE_DECLARE_FEATURE(kPageInfoHistoryDesktop);

// Hides site settings row.
BASE_DECLARE_FEATURE(kPageInfoHideSiteSettings);

#endif  // !BUILDFLAG(IS_ANDROID)

// Enables the merchant trust section for Page Info.
BASE_DECLARE_FEATURE(kMerchantTrust);

extern const char kMerchantTrustEnabledWithSampleDataName[];
extern const base::FeatureParam<bool> kMerchantTrustEnabledWithSampleData;

extern const char kMerchantTrustForceShowUIForTestingName[];
extern const base::FeatureParam<bool> kMerchantTrustForceShowUIForTesting;

// Whether the merchant trust section for Page Info based on country and locale.
extern bool IsMerchantTrustFeatureEnabled(const std::string& country_code,
                                          const std::string& locale);

}  // namespace page_info

#endif  // COMPONENTS_PAGE_INFO_CORE_FEATURES_H_

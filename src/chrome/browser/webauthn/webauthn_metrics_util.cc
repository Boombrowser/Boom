// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webauthn/webauthn_metrics_util.h"

#include "base/metrics/histogram_functions.h"

namespace {

// Maximum bucket for reporting number of passkeys present for a given
// Conditional UI request.
constexpr int kPasskeyCountMax = 10;

}  // namespace

void ReportConditionalUiPasskeyCount(int passkey_count) {
  base::UmaHistogramExactLinear("WebAuthentication.ConditionalUiPasskeyCount",
                                passkey_count, kPasskeyCountMax);
}

void RecordGPMMakeCredentialEvent(
    webauthn::metrics::GPMMakeCredentialEvents event) {
  base::UmaHistogramEnumeration("WebAuthentication.GPM.MakeCredential", event);
}

void RecordGPMGetAssertionEvent(
    webauthn::metrics::GPMGetAssertionEvents event) {
  base::UmaHistogramEnumeration("WebAuthentication.GPM.GetAssertion", event);
}

void RecordOnboardingEvent(webauthn::metrics::OnboardingEvents event) {
  base::UmaHistogramEnumeration("WebAuthentication.OnboardingEvents", event);
}

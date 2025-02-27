// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_CAPABILITIES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_CAPABILITIES_H_

#include "third_party/blink/renderer/modules/ai/ai_capability_availability.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/language_detection/language_detection_model.h"

namespace blink {

class AILanguageDetectorCapabilities final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  using LanguageDetectionModelStatus =
      language_detection::mojom::blink::LanguageDetectionModelStatus;
  explicit AILanguageDetectorCapabilities(
      LanguageDetectionModelStatus model_status);
  ~AILanguageDetectorCapabilities() override = default;

  V8AICapabilityAvailability available(ScriptState* script_state,
                                       ExceptionState& exception_state) const;
  V8AICapabilityAvailability languageAvailable(const WTF::String& languageTag);

 private:
  LanguageDetectionModelStatus model_status_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_AI_ON_DEVICE_TRANSLATION_AI_LANGUAGE_DETECTOR_CAPABILITIES_H_

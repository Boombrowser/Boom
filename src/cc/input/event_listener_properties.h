// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_EVENT_LISTENER_PROPERTIES_H_
#define CC_INPUT_EVENT_LISTENER_PROPERTIES_H_

#include <cstddef>

namespace cc {

enum class EventListenerClass {
  // This includes the pointerrawupdate events which are non-rAF-aligned.
  kPointerRawUpdate = 0,
  // This value includes "touchstart", "touchmove", and "pointer" events.
  kTouchStartOrMove,
  // This value includes "wheel" and "mousewheel" events.
  kMouseWheel,
  // This value includes "touchend" and "touchcancel" events.
  kTouchEndOrCancel,

  kLast = kTouchEndOrCancel
};

inline constexpr size_t kEventListenerClassCount =
    static_cast<size_t>(EventListenerClass::kLast) + 1;

enum class EventListenerProperties {
  kNone,
  kPassive,
  kBlocking,
  kBlockingAndPassive,
  kLast = kBlockingAndPassive
};

}  // namespace cc

#endif  // CC_INPUT_EVENT_LISTENER_PROPERTIES_H_

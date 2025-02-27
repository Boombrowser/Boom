// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_INPUT_WEB_KEYBOARD_EVENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_INPUT_WEB_KEYBOARD_EVENT_H_

#include <array>
#include <string>

#include "third_party/blink/public/common/input/web_input_event.h"

namespace blink {

// WebKeyboardEvent -----------------------------------------------------------

class BLINK_COMMON_EXPORT WebKeyboardEvent : public WebInputEvent {
 public:
  // Caps on string lengths so we can make them static arrays and keep
  // them PODs.
  static const size_t kTextLengthCap = 4;

  // |windows_key_code| is the Windows key code associated with this key
  // event. Sometimes it's direct from the event (i.e. on Windows),
  // sometimes it's via a mapping function. If you want a list, see
  // ui/events/keycodes/keyboard_codes_* . Note that this should
  // ALWAYS store the non-locational version of a keycode as this is
  // what is returned by the Windows API. For example, it should
  // store VK_SHIFT instead of VK_RSHIFT. The location information
  // should be stored in |modifiers|.
  int windows_key_code = 0;

  // The actual key code genenerated by the platform. The DOM spec runs
  // on Windows-equivalent codes (thus |windows_key_code| above) but it
  // doesn't hurt to have this one around.
  int native_key_code = 0;

  // The DOM code enum of the key pressed as passed by the embedder. DOM code
  // enums are defined in ui/events/keycodes/dom/dom_code_data.inc.
  int dom_code = 0;

  // The DOM key enum of the key pressed as passed by the embedder. DOM
  // key enums are defined in ui/events/keycodes/dom/dom_key_data.inc.
  uint32_t dom_key = 0;

  // This identifies whether this event was tagged by the system as being a
  // "system key" event (see
  // https://docs.microsoft.com/en-us/windows/desktop/inputdev/wm-syskeydown for
  // details). Other platforms don't have this concept, but it's just
  // easier to leave it always false than ifdef.
  bool is_system_key = false;

  // Whether the event forms part of a browser-handled keyboard shortcut.
  // This can be used to conditionally suppress Char events after a
  // shortcut-triggering RawKeyDown goes unhandled.
  bool is_browser_shortcut = false;

  // |text| is the text generated by this keystroke.  |unmodified_text| is
  // |text|, but unmodified by an concurrently-held modifiers (except
  // shift).  This is useful for working out shortcut keys.  Linux and
  // Windows guarantee one character per event.  The Mac does not, but in
  // reality that's all it ever gives.  We're generous, and cap it a bit
  // longer.
  std::array<char16_t, kTextLengthCap> text = {};
  std::array<char16_t, kTextLengthCap> unmodified_text = {};

  WebKeyboardEvent(Type type, int modifiers, base::TimeTicks time_stamp)
      : WebInputEvent(type, modifiers, time_stamp) {}

  WebKeyboardEvent() = default;

  std::unique_ptr<WebInputEvent> Clone() const override;
  bool CanCoalesce(const WebInputEvent& event) const override;
  void Coalesce(const WebInputEvent& event) override;

  // Please refer to bug http://b/issue?id=961192, which talks about Webkit
  // keyboard event handling changes. It also mentions the list of keys
  // which don't have associated character events.
  bool IsCharacterKey() const {
    // TODO(dtapuska): Determine if we can remove this method and just
    // not actually generate events for these instead of filtering them out.
    switch (windows_key_code) {
      case 0x08:  // VK_BACK
      case 0x1b:  // VK_ESCAPE
        return false;
    }
    return true;
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_INPUT_WEB_KEYBOARD_EVENT_H_

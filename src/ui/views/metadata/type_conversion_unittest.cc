// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/metadata/type_conversion.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/focus_ring.h"

using ViewsTypeConversionTest = PlatformTest;

TEST_F(ViewsTypeConversionTest, CheckIsSerializable) {
  // Test types with no explicit or aliased converters.
  EXPECT_FALSE(ui::metadata::TypeConverter<
               views::Button::PressedCallback>::is_serializable);
  EXPECT_FALSE(ui::metadata::TypeConverter<views::FocusRing*>::is_serializable);

  // Test std::optional type.
  EXPECT_FALSE(ui::metadata::TypeConverter<
               std::optional<views::FocusRing*>>::is_serializable);
}

// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.modules.stack_unwinder;

import org.chromium.components.module_installer.builder.ModuleInterface;

/**
 * Provides access to the stack unwinder native code functions within the dynamic feature module.
 */
@ModuleInterface(
        module = "stack_unwinder",
        impl = "org.chromium.chrome.modules.stack_unwinder.StackUnwinderModuleContentsImpl")
public interface StackUnwinderModuleContents {

    long getDoNothingFunction();
}

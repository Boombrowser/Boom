// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.browserservices.ui;

import androidx.annotation.Nullable;
import androidx.browser.trusted.TrustedWebActivityDisplayMode;
import androidx.browser.trusted.TrustedWebActivityDisplayMode.ImmersiveMode;

import org.chromium.base.supplier.Supplier;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.browserservices.trustedwebactivityui.controller.TrustedWebActivityBrowserControlsVisibilityManager;
import org.chromium.chrome.browser.browserservices.ui.controller.CurrentPageVerifier;
import org.chromium.chrome.browser.browserservices.ui.controller.CurrentPageVerifier.VerificationStatus;
import org.chromium.chrome.browser.browserservices.ui.controller.Verifier;
import org.chromium.chrome.browser.customtabs.CustomTabOrientationController;
import org.chromium.chrome.browser.customtabs.CustomTabStatusBarColorProvider;
import org.chromium.chrome.browser.customtabs.content.CustomTabActivityNavigationController;
import org.chromium.chrome.browser.customtabs.features.ImmersiveModeController;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbarColorController;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.lifecycle.InflationObserver;

/** Coordinator for shared functionality between Trusted Web Activities and webapps. */
public class SharedActivityCoordinator implements InflationObserver {
    private final CurrentPageVerifier mCurrentPageVerifier;
    private final TrustedWebActivityBrowserControlsVisibilityManager
            mBrowserControlsVisibilityManager;
    private final CustomTabToolbarColorController mToolbarColorController;
    private final CustomTabStatusBarColorProvider mStatusBarColorProvider;
    private final Supplier<ImmersiveModeController> mImmersiveModeController;
    private final CustomTabOrientationController mCustomTabOrientationController;

    @Nullable private final ImmersiveMode mImmersiveDisplayMode;

    private boolean mUseAppModeUi = true;

    public SharedActivityCoordinator(
            CurrentPageVerifier currentPageVerifier,
            TrustedWebActivityBrowserControlsVisibilityManager browserControlsVisibilityManager,
            CustomTabToolbarColorController toolbarColorController,
            CustomTabStatusBarColorProvider statusBarColorProvider,
            Supplier<ImmersiveModeController> immersiveModeController,
            BrowserServicesIntentDataProvider intentDataProvider,
            CustomTabOrientationController customTabOrientationController,
            CustomTabActivityNavigationController customTabActivityNavigationController,
            Verifier verifier,
            ActivityLifecycleDispatcher lifecycleDispatcher) {
        mCurrentPageVerifier = currentPageVerifier;
        mBrowserControlsVisibilityManager = browserControlsVisibilityManager;
        mToolbarColorController = toolbarColorController;
        mStatusBarColorProvider = statusBarColorProvider;
        mImmersiveModeController = immersiveModeController;
        mImmersiveDisplayMode = computeImmersiveMode(intentDataProvider);
        mCustomTabOrientationController = customTabOrientationController;

        customTabActivityNavigationController.setLandingPageOnCloseCriterion(
                verifier::wasPreviouslyVerified);

        mCurrentPageVerifier.addVerificationObserver(this::onVerificationUpdate);
        lifecycleDispatcher.register(this);
        if (mCurrentPageVerifier.getState() == null) {
            updateImmersiveMode(true); // Set immersive mode ASAP, before layout inflation.
        }
    }

    public boolean shouldUseAppModeUi() {
        return mUseAppModeUi;
    }

    @Override
    public void onPreInflationStartup() {}

    @Override
    public void onPostInflationStartup() {
        // Before the verification completes, we optimistically expect it to be successful and
        // apply the app mode UI.
        if (mCurrentPageVerifier.getState() == null) {
            updateUi(true);
        }
    }

    private void onVerificationUpdate() {
        CurrentPageVerifier.VerificationState state = mCurrentPageVerifier.getState();

        // The state will start off as null and progress to PENDING then SUCCESS/FAILURE.
        // We can show the app mode UI while the state is null or pending.
        boolean useAppModeUi = state == null || state.status != VerificationStatus.FAILURE;
        if (mUseAppModeUi == useAppModeUi) return;
        mUseAppModeUi = useAppModeUi;
        updateUi(useAppModeUi);
    }

    private void updateUi(boolean useAppModeUi) {
        updateImmersiveMode(useAppModeUi);
        mBrowserControlsVisibilityManager.updateIsInAppMode(useAppModeUi);
        mToolbarColorController.setUseTabThemeColor(useAppModeUi);
        mStatusBarColorProvider.setUseTabThemeColor(useAppModeUi);
        mCustomTabOrientationController.setCanControlOrientation(useAppModeUi);
    }

    private void updateImmersiveMode(boolean inAppMode) {
        if (mImmersiveDisplayMode == null) {
            return;
        }
        if (inAppMode) {
            mImmersiveModeController
                    .get()
                    .enterImmersiveMode(
                            mImmersiveDisplayMode.layoutInDisplayCutoutMode(),
                            mImmersiveDisplayMode.isSticky());
        } else {
            mImmersiveModeController.get().exitImmersiveMode();
        }
    }

    private ImmersiveMode computeImmersiveMode(
            BrowserServicesIntentDataProvider intentDataProvider) {
        TrustedWebActivityDisplayMode displayMode = intentDataProvider.getTwaDisplayMode();
        return (displayMode instanceof ImmersiveMode) ? (ImmersiveMode) displayMode : null;
    }
}

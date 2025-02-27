// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.usb;

import android.hardware.usb.UsbConfiguration;
import android.hardware.usb.UsbDevice;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

/**
 * Exposes android.hardware.usb.UsbDevice as necessary for C++ device::UsbDeviceAndroid.
 *
 * <p>Lifetime is controlled by device::UsbDeviceAndroid.
 */
@JNINamespace("device")
@NullMarked
final class ChromeUsbDevice {
    private static final String TAG = "Usb";

    final UsbDevice mDevice;

    private ChromeUsbDevice(UsbDevice device) {
        mDevice = device;
        Log.v(TAG, "ChromeUsbDevice created.");
    }

    public UsbDevice getDevice() {
        return mDevice;
    }

    @CalledByNative
    private static ChromeUsbDevice create(UsbDevice device) {
        return new ChromeUsbDevice(device);
    }

    @CalledByNative
    private int getDeviceId() {
        return mDevice.getDeviceId();
    }

    @CalledByNative
    private int getDeviceClass() {
        return mDevice.getDeviceClass();
    }

    @CalledByNative
    private int getDeviceSubclass() {
        return mDevice.getDeviceSubclass();
    }

    @CalledByNative
    private int getDeviceProtocol() {
        return mDevice.getDeviceProtocol();
    }

    @CalledByNative
    private int getVendorId() {
        return mDevice.getVendorId();
    }

    @CalledByNative
    private int getProductId() {
        return mDevice.getProductId();
    }

    @CalledByNative
    private int getDeviceVersion() {
        // The Android framework generates this string with:
        // Integer.toString(version >> 8) + "." + (version & 0xFF)
        //
        // This is not technically correct because the low nibble is actually
        // two separate version components (per spec). This undoes it at least.
        String[] parts = mDevice.getVersion().split("\\.");
        assert parts.length == 2;
        return Integer.parseInt(parts[0]) << 8 | Integer.parseInt(parts[1]);
    }

    @CalledByNative
    private @Nullable String getManufacturerName() {
        return mDevice.getManufacturerName();
    }

    @CalledByNative
    private @Nullable String getProductName() {
        return mDevice.getProductName();
    }

    @CalledByNative
    private @Nullable String getSerialNumber() {
        return mDevice.getSerialNumber();
    }

    @CalledByNative
    private UsbConfiguration[] getConfigurations() {
        int count = mDevice.getConfigurationCount();
        UsbConfiguration[] configurations = new UsbConfiguration[count];
        for (int i = 0; i < count; ++i) {
            configurations[i] = mDevice.getConfiguration(i);
        }
        return configurations;
    }
}

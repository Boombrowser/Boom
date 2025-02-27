// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(crbug.com/40285824): Remove this and convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "components/cronet/android/io_buffer_with_byte_buffer.h"

#include "base/check_op.h"
#include "base/numerics/safe_conversions.h"

namespace cronet {

IOBufferWithByteBuffer::IOBufferWithByteBuffer(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& jbyte_buffer,
    jint position,
    jint limit)
    : net::WrappedIOBuffer(
          base::span(static_cast<char*>(
                         env->GetDirectBufferAddress(jbyte_buffer.obj())),
                     base::checked_cast<size_t>(limit))
              .subspan(base::checked_cast<size_t>(position))),
      byte_buffer_(env, jbyte_buffer),
      initial_position_(position),
      initial_limit_(limit) {}

IOBufferWithByteBuffer::~IOBufferWithByteBuffer() = default;

ByteBufferWithIOBuffer::ByteBufferWithIOBuffer(
    JNIEnv* env,
    scoped_refptr<net::IOBuffer> io_buffer,
    int io_buffer_len)
    : io_buffer_(std::move(io_buffer)), io_buffer_len_(io_buffer_len) {
  // An intermediate ScopedJavaLocalRef is needed here to release the local
  // reference created by env->NewDirectByteBuffer().
  base::android::ScopedJavaLocalRef<jobject> java_buffer(
      env, env->NewDirectByteBuffer(io_buffer_->data(), io_buffer_len_));
  byte_buffer_.Reset(env, java_buffer.obj());
}

ByteBufferWithIOBuffer::~ByteBufferWithIOBuffer() = default;

}  // namespace cronet

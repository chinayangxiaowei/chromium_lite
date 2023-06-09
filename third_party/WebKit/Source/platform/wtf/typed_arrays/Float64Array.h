/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Float64Array_h
#define Float64Array_h

#include "platform/wtf/MathExtras.h"
#include "platform/wtf/typed_arrays/TypedArrayBase.h"

namespace WTF {

class Float64Array final : public TypedArrayBase<double> {
 public:
  static inline PassRefPtr<Float64Array> Create(unsigned length);
  static inline PassRefPtr<Float64Array> Create(const double* array,
                                                unsigned length);
  static inline PassRefPtr<Float64Array> Create(PassRefPtr<ArrayBuffer>,
                                                unsigned byte_offset,
                                                unsigned length);

  // Should only be used when it is known the entire array will be filled. Do
  // not return these results directly to JavaScript without filling first.
  static inline PassRefPtr<Float64Array> CreateUninitialized(unsigned length);

  using TypedArrayBase<double>::Set;

  void Set(unsigned index, double value) {
    if (index >= TypedArrayBase<double>::length_)
      return;
    TypedArrayBase<double>::Data()[index] = static_cast<double>(value);
  }

  ViewType GetType() const override { return kTypeFloat64; }

 private:
  inline Float64Array(PassRefPtr<ArrayBuffer>,
                      unsigned byte_offset,
                      unsigned length);
  // Make constructor visible to superclass.
  friend class TypedArrayBase<double>;
};

PassRefPtr<Float64Array> Float64Array::Create(unsigned length) {
  return TypedArrayBase<double>::Create<Float64Array>(length);
}

PassRefPtr<Float64Array> Float64Array::Create(const double* array,
                                              unsigned length) {
  return TypedArrayBase<double>::Create<Float64Array>(array, length);
}

PassRefPtr<Float64Array> Float64Array::Create(PassRefPtr<ArrayBuffer> buffer,
                                              unsigned byte_offset,
                                              unsigned length) {
  return TypedArrayBase<double>::Create<Float64Array>(std::move(buffer),
                                                      byte_offset, length);
}

Float64Array::Float64Array(PassRefPtr<ArrayBuffer> buffer,
                           unsigned byte_offset,
                           unsigned length)
    : TypedArrayBase<double>(std::move(buffer), byte_offset, length) {}

}  // namespace WTF

using WTF::Float64Array;

#endif  // Float64Array_h

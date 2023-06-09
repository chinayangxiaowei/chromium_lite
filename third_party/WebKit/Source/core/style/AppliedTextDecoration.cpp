// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/style/AppliedTextDecoration.h"

namespace blink {

AppliedTextDecoration::AppliedTextDecoration(TextDecoration line,
                                             TextDecorationStyle style,
                                             Color color)
    : lines_(static_cast<unsigned>(line)), style_(style), color_(color) {}

bool AppliedTextDecoration::operator==(const AppliedTextDecoration& o) const {
  return color_ == o.color_ && lines_ == o.lines_ && style_ == o.style_;
}

}  // namespace blink

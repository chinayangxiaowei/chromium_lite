// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResizeObserverEntry_h
#define ResizeObserverEntry_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Handle.h"

namespace blink {

class Element;
class ClientRect;
class LayoutRect;

class ResizeObserverEntry final : public GarbageCollected<ResizeObserverEntry>,
                                  public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ResizeObserverEntry(Element* target, const LayoutRect& content_rect);

  Element* target() const { return target_; }
  // FIXME(atotic): should return DOMRectReadOnly once https://crbug.com/388780
  // lands
  ClientRect* contentRect() const { return content_rect_; }

  DECLARE_VIRTUAL_TRACE();

 private:
  Member<Element> target_;
  Member<ClientRect> content_rect_;
};

}  // namespace blink

#endif  // ResizeObserverEntry_h

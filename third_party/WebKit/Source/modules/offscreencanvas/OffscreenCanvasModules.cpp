// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/offscreencanvas/OffscreenCanvasModules.h"

#include "core/dom/ExecutionContext.h"
#include "core/html/canvas/CanvasContextCreationAttributes.h"
#include "core/offscreencanvas/OffscreenCanvas.h"
#include "modules/offscreencanvas2d/OffscreenCanvasRenderingContext2D.h"

namespace blink {

void OffscreenCanvasModules::getContext(
    ScriptState* script_state,
    OffscreenCanvas& offscreen_canvas,
    const String& id,
    const CanvasContextCreationAttributes& attributes,
    ExceptionState& exception_state,
    OffscreenRenderingContext& result) {
  if (offscreen_canvas.IsNeutered()) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "OffscreenCanvas object is detached");
    return;
  }

  // OffscreenCanvas cannot be transferred after getContext, so this execution
  // context will always be the right one from here on.
  offscreen_canvas.SetExecutionContext(ExecutionContext::From(script_state));
  CanvasRenderingContext* context =
      offscreen_canvas.GetCanvasRenderingContext(script_state, id, attributes);
  if (context)
    context->SetOffscreenCanvasGetContextResult(result);
}

}  // namespace blink

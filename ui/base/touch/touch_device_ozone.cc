// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/touch/touch_device.h"

#include "base/logging.h"
#include "ui/events/devices/device_data_manager.h"

namespace ui {

bool IsTouchDevicePresent() {
  // TODO(sadrul@chromium.org): Support evdev hotplugging.
  return ui::DeviceDataManager::GetInstance()->touchscreen_devices().size() > 0;
}

int MaxTouchPoints() {
  // Hard-code this to 11 until we have a real implementation.
  return 11;
}

// TODO(mustaq@chromium.org): Use mouse detection logic. crbug.com/440503
int GetAvailablePointerTypes() {
  // Assume a mouse is there
  int available_pointer_types = POINTER_TYPE_FINE;
  if (IsTouchDevicePresent())
    available_pointer_types |= POINTER_TYPE_COARSE;

  DCHECK(available_pointer_types);
  return available_pointer_types;
}

PointerType GetPrimaryPointerType() {
  int available_pointer_types = GetAvailablePointerTypes();
  if (available_pointer_types & POINTER_TYPE_FINE)
    return POINTER_TYPE_FINE;
  if (available_pointer_types & POINTER_TYPE_COARSE)
    return POINTER_TYPE_COARSE;
  DCHECK_EQ(available_pointer_types, POINTER_TYPE_NONE);
  return POINTER_TYPE_NONE;
}

// TODO(mustaq@chromium.org): Use mouse detection logic. crbug.com/440503
int GetAvailableHoverTypes() {
  // Assume a mouse is there
  int available_hover_types = HOVER_TYPE_HOVER;
  if (IsTouchDevicePresent())
    available_hover_types |= HOVER_TYPE_ON_DEMAND;

  DCHECK(available_hover_types);
  return available_hover_types;
}

HoverType GetPrimaryHoverType() {
  int available_hover_types = GetAvailableHoverTypes();
  if (available_hover_types & HOVER_TYPE_HOVER)
    return HOVER_TYPE_HOVER;
  if (available_hover_types & HOVER_TYPE_ON_DEMAND)
    return HOVER_TYPE_ON_DEMAND;
  DCHECK_EQ(available_hover_types, HOVER_TYPE_NONE);
  return HOVER_TYPE_NONE;
}

}  // namespace ui

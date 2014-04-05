// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/panels/panel_browser_window_cocoa.h"

#include "base/logging.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/panels/panel.h"
#import "chrome/browser/ui/panels/panel_window_controller_cocoa.h"

namespace {

// Use this instead of 0 for minimum size of a window when doing opening and
// closing animations, since OSX window manager does not like 0-sized windows
// (according to avi@).
const int kMinimumWindowSize = 1;

// TODO(dcheng): Move elsewhere so BrowserWindowCocoa can use them too.
NSRect ConvertCoordinatesToCocoa(const gfx::Rect& bounds) {
  // Flip coordinates based on the primary screen.
  NSScreen* screen = [[NSScreen screens] objectAtIndex:0];

  return NSMakeRect(
      bounds.x(), NSHeight([screen frame]) - bounds.height() - bounds.y(),
      bounds.width(), bounds.height());
}

}  // namespace

// This creates a shim window class, which in turn creates a Cocoa window
// controller which in turn creates actual NSWindow by loading a nib.
// Overall chain of ownership is:
// PanelWindowControllerCocoa -> PanelBrowserWindowCocoa -> Panel.
NativePanel* Panel::CreateNativePanel(Browser* browser, Panel* panel,
                                      const gfx::Rect& bounds) {
  return new PanelBrowserWindowCocoa(browser, panel, bounds);
}

PanelBrowserWindowCocoa::PanelBrowserWindowCocoa(Browser* browser,
                                                 Panel* panel,
                                                 const gfx::Rect& bounds)
  : browser_(browser),
    panel_(panel),
    bounds_(bounds),
    is_shown_(false) {
  controller_ = [[PanelWindowControllerCocoa alloc] initWithBrowserWindow:this];
}

PanelBrowserWindowCocoa::~PanelBrowserWindowCocoa() {
}

bool PanelBrowserWindowCocoa::isClosed() {
  return !controller_;
}

void PanelBrowserWindowCocoa::ShowPanel() {
  if (isClosed())
    return;

  // Browser calls this several times, meaning 'ensure it's shown'.
  // Animations don't look good when repeated - hence this flag is needed.
  if (is_shown_) {
    return;
  }
  is_shown_ = true;

  NSRect finalFrame = ConvertCoordinatesToCocoa(GetPanelBounds());
  [controller_ revealAnimatedWithFrame:finalFrame];
}

void PanelBrowserWindowCocoa::ShowPanelInactive() {
  // TODO(dimich): to be implemented.
  ShowPanel();
}

gfx::Rect PanelBrowserWindowCocoa::GetPanelBounds() const {
  return bounds_;
}

void PanelBrowserWindowCocoa::SetPanelBounds(const gfx::Rect& bounds) {
  bounds_ = bounds;
  NSRect frame = ConvertCoordinatesToCocoa(bounds);
  [[controller_ window] setFrame:frame display:YES animate:YES];
}

void PanelBrowserWindowCocoa::OnPanelExpansionStateChanged(
    Panel::ExpansionState expansion_state) {
  NOTIMPLEMENTED();
}

bool PanelBrowserWindowCocoa::ShouldBringUpPanelTitleBar(int mouse_x,
                                                         int mouse_y) const {
  NOTIMPLEMENTED();
  return false;
}

void PanelBrowserWindowCocoa::ClosePanel() {
  if (isClosed())
      return;

  NSWindow* window = [controller_ window];
  NSRect frame = [window frame];
  frame.size.height = kMinimumWindowSize;
  // TODO(dimich): make this async. Currently, multiple panels will serially
  // (and annoyingly) close when user exits Chrome.
  [window setFrame:frame display:YES animate:YES];
  browser_->OnWindowClosing();
  DestroyPanelBrowser();  // not immediately, though.
}

void PanelBrowserWindowCocoa::ActivatePanel() {
  NOTIMPLEMENTED();
}

void PanelBrowserWindowCocoa::DeactivatePanel() {
  NOTIMPLEMENTED();
}

bool PanelBrowserWindowCocoa::IsPanelActive() const {
  NOTIMPLEMENTED();
  return false;
}

gfx::NativeWindow PanelBrowserWindowCocoa::GetNativePanelHandle() {
  return [controller_ window];
}

void PanelBrowserWindowCocoa::UpdatePanelTitleBar() {
  NOTIMPLEMENTED();
}

void PanelBrowserWindowCocoa::ShowTaskManagerForPanel() {
  NOTIMPLEMENTED();
}

void PanelBrowserWindowCocoa::NotifyPanelOnUserChangedTheme() {
  NOTIMPLEMENTED();
}

void PanelBrowserWindowCocoa::DrawAttention() {
  NOTIMPLEMENTED();
}

bool PanelBrowserWindowCocoa::IsDrawingAttention() const {
  NOTIMPLEMENTED();
  return false;
}

void PanelBrowserWindowCocoa::DestroyPanelBrowser() {
  [controller_ close];
  controller_ = NULL;
}

NativePanelTesting* PanelBrowserWindowCocoa::GetNativePanelTesting() {
  return this;
}

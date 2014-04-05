// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PANELS_PANEL_WINDOW_CONTROLLER_COCOA_H_
#define CHROME_BROWSER_UI_PANELS_PANEL_WINDOW_CONTROLLER_COCOA_H_

// A class acting as the Objective-C controller for the Panel window
// object. Handles interactions between Cocoa and the cross-platform
// code. Each window has a single titlebar and is managed/owned by Panel.

#import <Cocoa/Cocoa.h>

#import "base/mac/cocoa_protocols.h"
#include "base/memory/scoped_ptr.h"

class PanelBrowserWindowCocoa;
@class PanelTitlebarViewCocoa;

@interface PanelWindowControllerCocoa : NSWindowController<NSWindowDelegate> {
 @private
  IBOutlet PanelTitlebarViewCocoa* titlebar_view_;
  scoped_ptr<PanelBrowserWindowCocoa> windowShim_;
  NSView* mockTabContentsView_;  // Weak, only not-nil in unit_tests, where
                                 // it is inserted into the views hierarchy.
}

// Exposed for unit_test support. When called (from unit_test initialization),
// it allows for TabContents in the associated Browser to be not initialized,
// and substitutes the mock view instead of TabContent's one.
+ (void)enableMockTabContentsView;

// Load the browser window nib and do any Cocoa-specific initialization.
- (id)initWithBrowserWindow:(PanelBrowserWindowCocoa*)window;

// Returns the TabContents' native view. It renders the content of the web page
// in the Panel.
- (NSView*)tabContentsView;

// Sometimes (when we animate the size of the window) we want to stop resizing
// the TabContents's cocoa view to avoid unnecessary churn and issues
// that can be caused by sizes near 0.
- (void)disableTabContentsViewAutosizing;
- (void)enableTabContentsViewAutosizing;

// Shows the window for the first time. Only happens once.
- (void)revealAnimatedWithFrame:(const NSRect&)frame;

// Initiate the closing of the panel, starting from the platform-independent
// layer. This will take care of PanelManager, other panels and close the
// native window at the end.
- (void)closePanel;

// Accessor for titlebar view.
- (PanelTitlebarViewCocoa*)titlebarView;

@end  // @interface PanelWindowController

#endif  // CHROME_BROWSER_UI_PANELS_PANEL_WINDOW_CONTROLLER_COCOA_H_

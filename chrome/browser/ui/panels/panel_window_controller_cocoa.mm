// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/panels/panel_window_controller_cocoa.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/panels/panel.h"
#include "chrome/browser/ui/panels/panel_browser_window_cocoa.h"
#import "chrome/browser/ui/panels/panel_titlebar_view_cocoa.h"
#include "content/browser/tab_contents/tab_contents.h"

const int kMinimumWindowSize = 1;
static BOOL gIsMockTabContentsViewEnabled = NO;

@implementation PanelWindowControllerCocoa

- (id)initWithBrowserWindow:(PanelBrowserWindowCocoa*)window {
  NSString* nibpath =
      [base::mac::MainAppBundle() pathForResource:@"Panel" ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self]))
    windowShim_.reset(window);
  return self;
}

- (void)awakeFromNib {
  NSWindow* window = [self window];

  DCHECK(window);
  DCHECK(titlebar_view_);
  DCHECK_EQ(self, [window delegate]);

  // Using NSModalPanelWindowLevel (8) rather then NSStatusWindowLevel (25)
  // ensures notification balloons on top of regular windows, but below
  // popup menus which are at NSPopUpMenuWindowLevel (101) and Spotlight
  // drop-out, which is at NSStatusWindowLevel-2 (23) for OSX 10.6/7.
  // See http://crbug.com/59878.
  [window setLevel:NSModalPanelWindowLevel];

  [titlebar_view_ attach];

  // Attach the RenderWigetHostView to the view hierarchy, it will render
  // HTML content.
  [[window contentView] addSubview:[self tabContentsView]];
  [self enableTabContentsViewAutosizing];
}

- (void)disableTabContentsViewAutosizing {
  NSView* tabContentView = [self tabContentsView];
  DCHECK([tabContentView superview] == [[self window] contentView]);
  [tabContentView setAutoresizingMask:NSViewNotSizable];
}

- (void)enableTabContentsViewAutosizing {
  NSView* tabContentView = [self tabContentsView];
  DCHECK([tabContentView superview] == [[self window] contentView]);

  // Parent's bounds is child's frame.
  NSRect frame = [[[self window] contentView] bounds];
  [tabContentView setFrame:frame];
  [tabContentView
      setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
}

- (void)revealAnimatedWithFrame:(const NSRect&)frame {
  NSWindow* window = [self window];  // This ensures loading the nib.

  // Temporarily disable auto-resizing of the TabContents view to make animation
  // smoother and avoid rendering of HTML content at random intermediate sizes.
  [self disableTabContentsViewAutosizing];

  // We grow the window from the bottom up to produce a 'reveal' animation.
  NSRect startFrame = NSMakeRect(NSMinX(frame), NSMinY(frame),
                                 NSWidth(frame), kMinimumWindowSize);
  [window setFrame:startFrame display:NO animate:NO];
  // Shows the window without making it key, on top of its layer, even if
  // Chromium is not an active app.
  [window orderFrontRegardless];
  [window setFrame:frame display:YES animate:YES];

  // Resume auto-resizing of the TabContents view.
  [self enableTabContentsViewAutosizing];
}

- (void)closePanel {
  windowShim_->panel()->Close();
}

- (void)windowWillClose:(NSNotification*)notification {
  [self autorelease];
}

- (NSView*)tabContentsView {
  TabContents* contents = windowShim_->browser()->GetSelectedTabContents();
  if (contents) {
    NSView* tabContentView = contents->GetNativeView();
    DCHECK(tabContentView);
    return tabContentView;
  } else {
    // This is the UNIT_TEST situation. In unit_tests, there is no navigation
    // and no TabContents created. Lets make sure we are in a unit_test by
    // checking the flag set only by the unit_tests, and then return an NSView
    // which will mock the tab_content_view.
    CHECK(gIsMockTabContentsViewEnabled);
    if (!mockTabContentsView_)
      mockTabContentsView_ = [[NSView alloc] initWithFrame:NSZeroRect];
    return mockTabContentsView_;
  }
}

- (PanelTitlebarViewCocoa*)titlebarView {
  return titlebar_view_;
}

+ (void)enableMockTabContentsView {
  gIsMockTabContentsViewEnabled = YES;
}
@end

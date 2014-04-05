// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_WINDOW_GTK_H_
#define CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_WINDOW_GTK_H_

#include "chrome/browser/ui/gtk/browser_window_gtk.h"
#include "chrome/browser/ui/panels/native_panel.h"

class Panel;
class NativePanelTestingGtk;

class PanelBrowserWindowGtk : public BrowserWindowGtk,
                              public NativePanel,
                              public MessageLoopForUI::Observer {
  friend class NativePanelTestingGtk;
 public:
  PanelBrowserWindowGtk(Browser* browser, Panel* panel,
                        const gfx::Rect& bounds);
  virtual ~PanelBrowserWindowGtk();

  // BrowserWindowGtk overrides
  virtual void Init() OVERRIDE;

  // BrowserWindow overrides
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;

 protected:
  // BrowserWindowGtk overrides
  virtual bool GetWindowEdge(int x, int y, GdkWindowEdge* edge) OVERRIDE;
  virtual bool HandleTitleBarLeftMousePress(
      GdkEventButton* event,
      guint32 last_click_time,
      gfx::Point last_click_position) OVERRIDE;
  virtual void SaveWindowPosition() OVERRIDE;
  virtual void SetGeometryHints() OVERRIDE;
  virtual bool UseCustomFrame() OVERRIDE;

  // Overridden from NativePanel:
  virtual void ShowPanel() OVERRIDE;
  virtual void ShowPanelInactive() OVERRIDE;
  virtual gfx::Rect GetPanelBounds() const OVERRIDE;
  virtual void SetPanelBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual void OnPanelExpansionStateChanged(
      Panel::ExpansionState expansion_state) OVERRIDE;
  virtual bool ShouldBringUpPanelTitlebar(int mouse_x,
                                          int mouse_y) const OVERRIDE;
  virtual void ClosePanel() OVERRIDE;
  virtual void ActivatePanel() OVERRIDE;
  virtual void DeactivatePanel() OVERRIDE;
  virtual bool IsPanelActive() const OVERRIDE;
  virtual gfx::NativeWindow GetNativePanelHandle() OVERRIDE;
  virtual void UpdatePanelTitleBar() OVERRIDE;
  virtual void ShowTaskManagerForPanel() OVERRIDE;
  virtual FindBar* CreatePanelFindBar() OVERRIDE;
  virtual void NotifyPanelOnUserChangedTheme() OVERRIDE;
  virtual void DrawAttention() OVERRIDE;
  virtual bool IsDrawingAttention() const OVERRIDE;
  virtual bool PreHandlePanelKeyboardEvent(
      const NativeWebKeyboardEvent& event,
      bool* is_keyboard_shortcut) OVERRIDE;
  virtual void HandlePanelKeyboardEvent(
      const NativeWebKeyboardEvent& event) OVERRIDE;
  virtual Browser* GetPanelBrowser() const OVERRIDE;
  virtual void DestroyPanelBrowser() OVERRIDE;
  virtual gfx::Size GetNonClientAreaExtent() const OVERRIDE;
  virtual int GetRestoredHeight() const OVERRIDE;
  virtual void SetRestoredHeight(int height) OVERRIDE;

 private:
  void SetBoundsImpl();

  // MessageLoop::Observer implementation:
  virtual void WillProcessEvent(GdkEvent* event) OVERRIDE;
  virtual void DidProcessEvent(GdkEvent* event) OVERRIDE;

  void CreateDragWidget();
  void DestroyDragWidget();
  void EndDrag(bool canceled);
  void CleanupDragDrop();

  CHROMEGTK_CALLBACK_1(PanelBrowserWindowGtk, gboolean,
                       OnTitlebarButtonPressEvent, GdkEventButton*);
  CHROMEGTK_CALLBACK_1(PanelBrowserWindowGtk, gboolean,
                       OnTitlebarButtonReleaseEvent, GdkEventButton*);

  // drag-begin is emitted when the drag is started. We connect so that we can
  // set the drag icon to a transparent pixbuf.
  CHROMEGTK_CALLBACK_1(PanelBrowserWindowGtk, void, OnDragBegin,
                       GdkDragContext*);

  // drag-failed is emitted when the drag is finished.  In our case the signal
  // does not imply failure as we don't use the drag-n-drop API to transfer drop
  // data.
  CHROMEGTK_CALLBACK_2(PanelBrowserWindowGtk, gboolean, OnDragFailed,
                       GdkDragContext*, GtkDragResult);

  // When a drag is ending, a fake button release event is passed to the drag
  // widget to fake letting go of the mouse button.  We need a callback for
  // this event because it is the only way to catch drag end events when the
  // user presses space or return.
  CHROMEGTK_CALLBACK_1(PanelBrowserWindowGtk, gboolean, OnDragButtonReleased,
                       GdkEventButton*);

  // A copy of the last button press event, used to initiate a drag.
  GdkEvent* last_mouse_down_;

  // A GtkInivisible used to track the drag event.  GtkInvisibles are of the
  // type GInitiallyUnowned, but the widget initialization code sinks the
  // reference, so we can't use an OwnedWidgetGtk here.
  GtkWidget* drag_widget_;

  // Used to destroy the drag widget after a return to the message loop.
  ScopedRunnableMethodFactory<PanelBrowserWindowGtk>
      destroy_drag_widget_factory_;

  // Due to a bug in GTK+, we need to force the end of a drag when we get a
  // mouse release event on the the dragged widget, otherwise, we don't know
  // when the drag has ended when the user presses space or enter.  We queue
  // a task to end the drag and only run it if GTK+ didn't send us the
  // drag-failed event.
  ScopedRunnableMethodFactory<PanelBrowserWindowGtk> drag_end_factory_;

  scoped_ptr<Panel> panel_;
  gfx::Rect bounds_;
  DISALLOW_COPY_AND_ASSIGN(PanelBrowserWindowGtk);
};

#endif  // CHROME_BROWSER_UI_PANELS_PANEL_BROWSER_WINDOW_GTK_H_

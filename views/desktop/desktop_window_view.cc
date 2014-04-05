// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/desktop/desktop_window_view.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/transform.h"
#include "views/desktop/desktop_background.h"
#include "views/desktop/desktop_window_root_view.h"
#include "views/layer_property_setter.h"
#include "views/widget/native_widget_view.h"
#include "views/widget/native_widget_views.h"
#include "views/widget/widget.h"
#include "views/window/native_frame_view.h"

#if defined(OS_WIN)
#include "views/widget/native_widget_win.h"
#elif defined(TOOLKIT_USES_GTK)
#include "views/widget/native_widget_gtk.h"
#endif

namespace views {
namespace desktop {

// The Widget that hosts the DesktopWindowView. Subclasses Widget to override
// CreateRootView() so that the DesktopWindowRootView can be supplied instead
// for custom event filtering.
class DesktopWindow : public Widget {
 public:
  explicit DesktopWindow(DesktopWindowView* desktop_window_view)
      : desktop_window_view_(desktop_window_view) {}
  virtual ~DesktopWindow() {}

 private:
  // Overridden from Widget:
  virtual internal::RootView* CreateRootView() OVERRIDE {
    return new DesktopWindowRootView(desktop_window_view_, this);
  }

  virtual bool OnKeyEvent(const KeyEvent& event) OVERRIDE {
    NativeWidgetViews* native_widget =
        desktop_window_view_->active_native_widget();
    return native_widget ? native_widget->OnKeyEvent(event) : false;
  }

  DesktopWindowView* desktop_window_view_;

  DISALLOW_COPY_AND_ASSIGN(DesktopWindow);
};

class TestWindowContentView : public WidgetDelegateView {
 public:
  TestWindowContentView(const std::wstring& title, SkColor color)
      : title_(title),
        color_(color) {
  }
  virtual ~TestWindowContentView() {}

 private:
  // Overridden from View:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE {
    canvas->FillRectInt(color_, 0, 0, width(), height());
  }

  // Overridden from WindowDelegate:
  virtual std::wstring GetWindowTitle() const OVERRIDE {
    return title_;
  }
  virtual View* GetContentsView() {
    return this;
  }
  virtual bool CanMaximize() const OVERRIDE {
    return true;
  }
  virtual bool OnMousePressed(const MouseEvent& event) OVERRIDE {
    Widget* widget = View::GetWidget();
    if (widget->IsMinimized())
      widget->Restore();
    else
      widget->Minimize();
    return true;
  }

  std::wstring title_;
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(TestWindowContentView);
};

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowView, public:

// static
DesktopWindowView* DesktopWindowView::desktop_window_view = NULL;

DesktopWindowView::DesktopWindowView(DesktopType type)
    : active_native_widget_(NULL),
      type_(type) {
  switch (type_) {
    case DESKTOP_DEFAULT:
    case DESKTOP_NETBOOK:
      set_background(new DesktopBackground);
      break;
    case DESKTOP_OTHER:
      set_background(Background::CreateStandardPanelBackground());
      break;
  }
}

DesktopWindowView::~DesktopWindowView() {
}

// static
void DesktopWindowView::CreateDesktopWindow(DesktopType type) {
  DCHECK(!desktop_window_view);
  desktop_window_view = new DesktopWindowView(type);
  views::Widget* window = new DesktopWindow(desktop_window_view);
  desktop_window_view->widget_ = window;

  views::Widget::InitParams params;
  params.delegate = desktop_window_view;
  // In this environment, CreateChromeWindow will default to creating a views-
  // window, so we need to construct a NativeWidgetWin by hand.
  // TODO(beng): Replace this with NativeWindow::CreateNativeRootWindow().
#if defined(OS_WIN)
  params.native_widget = new views::NativeWidgetWin(window);
  params.bounds = gfx::Rect(20, 20, 1920, 1200);
#elif defined(TOOLKIT_USES_GTK)
  params.native_widget = new views::NativeWidgetGtk(window);
#endif
  window->Init(params);
  window->Show();
  window->Maximize();
}

void DesktopWindowView::ActivateWidget(Widget* widget) {
  if (widget && widget->IsActive())
    return;

  if (active_native_widget_)
    active_native_widget_->OnActivate(false);
  if (widget) {
    widget->MoveToTop();
    active_native_widget_ =
        static_cast<NativeWidgetViews*>(widget->native_widget());
    active_native_widget_->OnActivate(true);
    if (!widget->HasObserver(this))
      widget->AddObserver(this);
  }
}


void DesktopWindowView::CreateTestWindow(const std::wstring& title,
                                         SkColor color,
                                         gfx::Rect initial_bounds,
                                         bool rotate) {
  views::Widget* window = views::Widget::CreateWindowWithBounds(
      new TestWindowContentView(title, color),
      initial_bounds);
  window->Show();

  if (rotate) {
    ui::Transform transform;
    transform.SetRotate(90.0f);
    transform.SetTranslateX(window->GetWindowScreenBounds().width());
    static_cast<NativeWidgetViews*>(window->native_widget())->GetView()->
        SetTransform(transform);
  }
  static_cast<NativeWidgetViews*>(window->native_widget())->GetView()->
      SetLayerPropertySetter(LayerPropertySetter::CreateAnimatingSetter());
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowView, View overrides:

void DesktopWindowView::Layout() {
}

void DesktopWindowView::ViewHierarchyChanged(
    bool is_add, View* parent, View* child) {
  if (!is_add &&
      active_native_widget_ &&
      active_native_widget_->GetView() == child)
    active_native_widget_ = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowView, WidgetDelegate implementation:

Widget* DesktopWindowView::GetWidget() {
  return widget_;
}

const Widget* DesktopWindowView::GetWidget() const {
  return widget_;
}

bool DesktopWindowView::CanResize() const {
  return true;
}

bool DesktopWindowView::CanMaximize() const {
  return CanResize();
}

std::wstring DesktopWindowView::GetWindowTitle() const {
  return L"Aura Desktop";
}

SkBitmap DesktopWindowView::GetWindowAppIcon() {
  return SkBitmap();
}

SkBitmap DesktopWindowView::GetWindowIcon() {
  return SkBitmap();
}

bool DesktopWindowView::ShouldShowWindowIcon() const {
  return false;
}

void DesktopWindowView::WindowClosing() {
  MessageLoopForUI::current()->Quit();
}

View* DesktopWindowView::GetContentsView() {
  return this;
}

NonClientFrameView* DesktopWindowView::CreateNonClientFrameView() {
  switch (type_) {
    case DESKTOP_DEFAULT:
    case DESKTOP_NETBOOK:
      return NULL;

    case DESKTOP_OTHER:
      return new NativeFrameView(widget_);
  }
  return NULL;
}

void DesktopWindowView::OnWidgetClosing(Widget* widget) {
  if (active_native_widget_ && static_cast<internal::NativeWidgetPrivate*>
      (active_native_widget_)->GetWidget() == widget)
    active_native_widget_ = NULL;
}

void DesktopWindowView::OnWidgetVisibilityChanged(Widget* widget,
                                                  bool visible) {
}

void DesktopWindowView::OnWidgetActivationChanged(Widget* widget,
                                                  bool active) {
}

}  // namespace desktop
}  // namespace views

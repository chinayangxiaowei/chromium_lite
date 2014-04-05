// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "aura/desktop.h"

#include "aura/desktop_host.h"
#include "aura/root_window.h"
#include "aura/window.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "ui/gfx/compositor/compositor.h"

namespace aura {

// static
Desktop* Desktop::instance_ = NULL;

Desktop::Desktop()
    : host_(aura::DesktopHost::Create(gfx::Rect(200, 200, 1024, 768))) {
  compositor_ = ui::Compositor::Create(host_->GetAcceleratedWidget(),
                                       host_->GetSize());
  host_->SetDesktop(this);
  DCHECK(compositor_.get());
  window_.reset(new internal::RootWindow);
}

Desktop::~Desktop() {
}

void Desktop::Show() {
  host_->Show();
}

void Desktop::SetSize(const gfx::Size& size) {
  host_->SetSize(size);
}

void Desktop::Run() {
  Show();
  MessageLoop main_message_loop(MessageLoop::TYPE_UI);
  MessageLoopForUI::current()->Run(host_);
}

void Desktop::Draw() {
  compositor_->NotifyStart();
  window_->DrawTree();
  compositor_->NotifyEnd();
}

bool Desktop::OnMouseEvent(const MouseEvent& event) {
  return window_->HandleMouseEvent(event);
}

bool Desktop::OnKeyEvent(const KeyEvent& event) {
  return window_->HandleKeyEvent(event);
}

// static
Desktop* Desktop::GetInstance() {
  if (!instance_) {
    instance_ = new Desktop;
    instance_->window_->Init();
  }
  return instance_;
}

}  // namespace aura

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEBUGGER_DEVTOOLS_WINDOW_H_
#define CHROME_BROWSER_DEBUGGER_DEVTOOLS_WINDOW_H_
#pragma once

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "chrome/browser/debugger/devtools_toggle_action.h"
#include "content/browser/debugger/devtools_client_host.h"
#include "content/browser/tab_contents/tab_contents_delegate.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

namespace IPC {
class Message;
}

class Browser;
class BrowserWindow;
class PrefService;
class Profile;
class RenderViewHost;
class TabContentsWrapper;

namespace base {
class Value;
}

class DevToolsWindow
    : public DevToolsClientHost,
      public NotificationObserver,
      public TabContentsDelegate {
 public:
  static const char kDevToolsApp[];
  static void RegisterUserPrefs(PrefService* prefs);
  static TabContentsWrapper* GetDevToolsContents(TabContents* inspected_tab);
  static DevToolsWindow* FindDevToolsWindow(RenderViewHost* window_rvh);

  static DevToolsWindow* CreateDevToolsWindowForWorker(Profile* profile);
  static DevToolsWindow* OpenDevToolsWindow(RenderViewHost* inspected_rvh);
  static DevToolsWindow* ToggleDevToolsWindow(RenderViewHost* inspected_rvh,
                                              DevToolsToggleAction action);
  static void InspectElement(RenderViewHost* inspected_rvh, int x, int y);

  virtual ~DevToolsWindow();

  // Overridden from DevToolsClientHost.
  virtual void SendMessageToClient(const IPC::Message& message);
  virtual void InspectedTabClosing();
  virtual void TabReplaced(TabContents* new_tab);
  virtual RenderViewHost* GetClientRenderViewHost();
  virtual void RequestActivate();
  virtual void RequestSetDocked(bool docked);
  virtual void RequestClose();
  virtual void RequestSaveAs(const std::string& suggested_file_name,
                             const std::string& content);
  RenderViewHost* GetRenderViewHost();

  void Show(DevToolsToggleAction action);

  TabContentsWrapper* tab_contents() { return tab_contents_; }
  Browser* browser() { return browser_; }  // For tests.
  bool is_docked() { return docked_; }

 private:
  DevToolsWindow(Profile* profile, RenderViewHost* inspected_rvh, bool docked,
                 bool shared_worker_frontend);

  void CreateDevToolsBrowser();
  bool FindInspectedBrowserAndTabIndex(Browser**, int* tab);
  BrowserWindow* GetInspectedBrowserWindow();
  bool IsInspectedBrowserPopupOrPanel();
  void UpdateFrontendAttachedState();

  // Overridden from NotificationObserver.
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  void ScheduleAction(DevToolsToggleAction action);
  void DoAction();
  GURL GetDevToolsUrl();
  void UpdateTheme();
  void AddDevToolsExtensionsToClient();
  void CallClientFunction(const string16& function_name,
                          const base::Value& arg);
  // Overridden from TabContentsDelegate.
  virtual TabContents* OpenURLFromTab(
      TabContents* source,
      const GURL& url,
      const GURL& referrer,
      WindowOpenDisposition disposition,
      PageTransition::Type transition);
  virtual void AddNewContents(TabContents* source,
                              TabContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture);
  virtual void CloseContents(TabContents* source) {}
  virtual bool CanReloadContents(TabContents* source) const;
  virtual bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                                      bool* is_keyboard_shortcut);
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event);
  virtual content::JavaScriptDialogCreator* GetJavaScriptDialogCreator();

  virtual void FrameNavigating(const std::string& url) {}

  static DevToolsWindow* ToggleDevToolsWindow(RenderViewHost* inspected_rvh,
                                              bool force_open,
                                              DevToolsToggleAction action);
  static DevToolsWindow* AsDevToolsWindow(DevToolsClientHost*);

  Profile* profile_;
  TabContentsWrapper* inspected_tab_;
  TabContentsWrapper* tab_contents_;
  Browser* browser_;
  bool docked_;
  bool is_loaded_;
  DevToolsToggleAction action_on_load_;
  const bool shared_worker_frontend_;
  NotificationRegistrar registrar_;
  DISALLOW_COPY_AND_ASSIGN(DevToolsWindow);
};

#endif  // CHROME_BROWSER_DEBUGGER_DEVTOOLS_WINDOW_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_TABS_ASH_PANEL_CONTENTS_H_
#define CHROME_BROWSER_EXTENSIONS_API_TABS_ASH_PANEL_CONTENTS_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/ui/ash/launcher/launcher_favicon_loader.h"
#include "content/public/browser/web_contents_observer.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/extension_function_dispatcher.h"

class AshPanelWindowController;
class GURL;

namespace content {
class RenderViewHost;
}

namespace extensions {
struct DraggableRegion;
}

// extensions::AppWindowContents class specific to panel windows created by v1
// extenstions. This class maintains a WebContents instance and observes it for
// the purpose of passing messages to the extensions system. It also creates
// an extensions::WindowController instance for interfacing with the v1
// extensions API.
class AshPanelContents
    : public extensions::AppWindowContents,
      public content::WebContentsObserver,
      public LauncherFaviconLoader::Delegate,
      public extensions::ExtensionFunctionDispatcher::Delegate {
 public:
  explicit AshPanelContents(extensions::AppWindow* host);
  ~AshPanelContents() override;

  // extensions::AppWindowContents
  void Initialize(content::BrowserContext* context, const GURL& url) override;
  void LoadContents(int32 creator_process_id) override;
  void NativeWindowChanged(
      extensions::NativeAppWindow* native_app_window) override;
  void NativeWindowClosed() override;
  void DispatchWindowShownForTests() const override;
  content::WebContents* GetWebContents() const override;

  // LauncherFaviconLoader::Delegate overrides:
  void FaviconUpdated() override;

  // extensions::ExtensionFunctionDispatcher::Delegate
  extensions::WindowController* GetExtensionWindowController() const override;
  content::WebContents* GetAssociatedWebContents() const override;

  LauncherFaviconLoader* launcher_favicon_loader_for_test() {
    return launcher_favicon_loader_.get();
  }

 private:
  // content::WebContentsObserver
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnRequest(const ExtensionHostMsg_Request_Params& params);

  extensions::AppWindow* host_;
  GURL url_;
  scoped_ptr<content::WebContents> web_contents_;
  scoped_ptr<extensions::ExtensionFunctionDispatcher>
      extension_function_dispatcher_;
  scoped_ptr<AshPanelWindowController> window_controller_;
  scoped_ptr<LauncherFaviconLoader> launcher_favicon_loader_;

  DISALLOW_COPY_AND_ASSIGN(AshPanelContents);
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_TABS_ASH_PANEL_CONTENTS_H_

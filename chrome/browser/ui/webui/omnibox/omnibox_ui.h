// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OMNIBOX_OMNIBOX_UI_H_
#define CHROME_BROWSER_UI_WEBUI_OMNIBOX_OMNIBOX_UI_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/mojo_web_ui_controller.h"
#include "chrome/browser/ui/webui/omnibox/omnibox.mojom.h"

class OmniboxPageHandler;

// The UI for chrome://omnibox/
class OmniboxUI : public MojoWebUIController<mojom::OmniboxPageHandler> {
 public:
  explicit OmniboxUI(content::WebUI* contents);
  ~OmniboxUI() override;

 private:
  // MojoWebUIController overrides:
  void BindUIHandler(const service_manager::BindSourceInfo& source_info,
                     mojom::OmniboxPageHandlerRequest request) override;

  std::unique_ptr<OmniboxPageHandler> omnibox_handler_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_OMNIBOX_OMNIBOX_UI_H_

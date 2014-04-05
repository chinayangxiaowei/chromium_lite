// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NTP_BOOKMARKS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_NTP_BOOKMARKS_HANDLER_H_
#pragma once

#include "content/browser/webui/web_ui.h"
#include "content/common/notification_observer.h"

// The handler for Javascript messages related to the "bookmarks" view.
class BookmarksHandler : public WebUIMessageHandler,
                         public NotificationObserver {
 public:
  explicit BookmarksHandler();
  virtual ~BookmarksHandler();

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages();

  // NotificationObserver
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarksHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NTP_BOOKMARKS_HANDLER_H_

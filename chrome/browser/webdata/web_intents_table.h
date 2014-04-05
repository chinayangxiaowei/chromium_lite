// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBDATA_WEB_INTENTS_TABLE_H_
#define CHROME_BROWSER_WEBDATA_WEB_INTENTS_TABLE_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/string16.h"
#include "chrome/browser/intents/web_intent_data.h"
#include "chrome/browser/webdata/web_database_table.h"

class GURL;

namespace sql {
class Connection;
class MetaTable;
}

// This class manages the WebIntents table within the SQLite database passed
// to the constructor. It expects the following schema:
//
// web_intents
//    service_url       URL for service invocation.
//    action            Name of action provided by the service.
//    type              MIME type of data accepted by the service.
//
// Intents are uniquely identified by the <service_url,action,type> tuple.
class WebIntentsTable : public WebDatabaseTable {
 public:
   WebIntentsTable(sql::Connection* db, sql::MetaTable* meta_table);
   virtual ~WebIntentsTable();

  // WebDatabaseTable implementation.
  virtual bool Init();
  virtual bool IsSyncable();

  // Adds a web intent to the WebIntents table. If intent already exists,
  // replaces it.
  bool SetWebIntent(const WebIntentData& intent);

  // Retrieve all intents from WebIntents table that match |action|.
  bool GetWebIntents(const string16& action,
                     std::vector<WebIntentData>* intents);

  // Retrieve all intents from WebIntents table.
  bool GetAllWebIntents(std::vector<WebIntentData>* intents);

  // Removes intent from WebIntents table - must match all parameters exactly.
  bool RemoveWebIntent(const WebIntentData& intent);

 private:
  DISALLOW_COPY_AND_ASSIGN(WebIntentsTable);
};

#endif  // CHROME_BROWSER_WEBDATA_WEB_INTENTS_TABLE_H_

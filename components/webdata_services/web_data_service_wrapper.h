// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_
#define COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/keyed_service.h"
#include "sql/init_status.h"
#include "sync/api/syncable_service.h"

class KeywordWebDataService;
class TokenWebData;
class WebDatabaseService;

#if defined(OS_WIN)
class PasswordWebDataService;
#endif

namespace autofill {
class AutofillWebDataBackend;
class AutofillWebDataService;
}  // namespace autofill

namespace base {
class FilePath;
class MessageLoopProxy;
}  // namespace base

// WebDataServiceWrapper is a KeyedService that owns multiple WebDataServices
// so that they can be associated with a context.
class WebDataServiceWrapper : public KeyedService {
 public:
  // ErrorType indicates which service encountered an error loading its data.
  enum ErrorType {
    ERROR_LOADING_AUTOFILL,
    ERROR_LOADING_KEYWORD,
    ERROR_LOADING_TOKEN,
    ERROR_LOADING_PASSWORD,
  };

  // Shows an error message if a loading error occurs.
  using ShowErrorCallback = void (*)(ErrorType, sql::InitStatus);

  // Constructor for WebDataServiceWrapper that initializes the different
  // WebDataServices and starts the synchronization services using |flare|.
  // Since |flare| will be copied and called multiple times, it cannot bind
  // values using base::Owned nor base::Passed; it should only bind simple or
  // refcounted types.
  WebDataServiceWrapper(const base::FilePath& context_path,
                        const std::string& application_locale,
                        const scoped_refptr<base::MessageLoopProxy>& ui_thread,
                        const scoped_refptr<base::MessageLoopProxy>& db_thread,
                        const syncer::SyncableService::StartSyncFlare& flare,
                        ShowErrorCallback show_error_callback);
  ~WebDataServiceWrapper() override;

  // For testing.
  WebDataServiceWrapper();

  // KeyedService:
  void Shutdown() override;

  // Create the various types of service instances.  These methods are virtual
  // for testing purpose.
  virtual scoped_refptr<autofill::AutofillWebDataService> GetAutofillWebData();
  virtual scoped_refptr<KeywordWebDataService> GetKeywordWebData();
  virtual scoped_refptr<TokenWebData> GetTokenWebData();
#if defined(OS_WIN)
  virtual scoped_refptr<PasswordWebDataService> GetPasswordWebData();
#endif

 private:
  scoped_refptr<WebDatabaseService> web_database_;

  scoped_refptr<autofill::AutofillWebDataService> autofill_web_data_;
  scoped_refptr<KeywordWebDataService> keyword_web_data_;
  scoped_refptr<TokenWebData> token_web_data_;

#if defined(OS_WIN)
  scoped_refptr<PasswordWebDataService> password_web_data_;
#endif

  DISALLOW_COPY_AND_ASSIGN(WebDataServiceWrapper);
};

#endif  // COMPONENTS_WEBDATA_SERVICES_WEB_DATA_SERVICE_WRAPPER_H_

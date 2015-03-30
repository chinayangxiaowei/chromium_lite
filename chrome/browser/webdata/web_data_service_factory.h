// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBDATA_WEB_DATA_SERVICE_FACTORY_H__
#define CHROME_BROWSER_WEBDATA_WEB_DATA_SERVICE_FACTORY_H__

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

template <typename T> struct DefaultSingletonTraits;
class KeywordWebDataService;
class TokenWebData;
class WebDataServiceWrapper;

#if defined(OS_WIN)
class PasswordWebDataService;
#endif

namespace autofill {
class AutofillWebDataBackend;
class AutofillWebDataService;
}  // namespace autofill

// Singleton that owns all WebDataServiceWrappers and associates them with
// Profiles.
class WebDataServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the WebDataServiceWrapper associated with the |profile|.
  static WebDataServiceWrapper* GetForProfile(
      Profile* profile,
      Profile::ServiceAccessType access_type);

  static WebDataServiceWrapper* GetForProfileIfExists(
      Profile* profile,
      Profile::ServiceAccessType access_type);

  // Returns the AutofillWebDataService associated with the |profile|.
  static scoped_refptr<autofill::AutofillWebDataService>
      GetAutofillWebDataForProfile(Profile* profile,
                                   Profile::ServiceAccessType access_type);

  // Returns the KeywordWebDataService associated with the |profile|.
  static scoped_refptr<KeywordWebDataService> GetKeywordWebDataForProfile(
      Profile* profile,
      Profile::ServiceAccessType access_type);

  // Returns the TokenWebData associated with the |profile|.
  static scoped_refptr<TokenWebData> GetTokenWebDataForProfile(
      Profile* profile,
      Profile::ServiceAccessType access_type);

#if defined(OS_WIN)
  // Returns the PasswordWebDataService associated with the |profile|.
  static scoped_refptr<PasswordWebDataService> GetPasswordWebDataForProfile(
      Profile* profile,
      Profile::ServiceAccessType access_type);
#endif

  static WebDataServiceFactory* GetInstance();

 private:
  friend struct DefaultSingletonTraits<WebDataServiceFactory>;

  WebDataServiceFactory();
  ~WebDataServiceFactory() override;

  // |BrowserContextKeyedBaseFactory| methods:
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  bool ServiceIsNULLWhileTesting() const override;

  DISALLOW_COPY_AND_ASSIGN(WebDataServiceFactory);
};

#endif  // CHROME_BROWSER_WEBDATA_WEB_DATA_SERVICE_FACTORY_H__

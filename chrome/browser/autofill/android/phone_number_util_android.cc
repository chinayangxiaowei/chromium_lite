// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/android/phone_number_util_android.h"

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "chrome/browser/browser_process.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "jni/PhoneNumberUtil_jni.h"
#include "third_party/libphonenumber/phonenumber_api.h"

namespace autofill {

namespace {
using ::base::android::ConvertJavaStringToUTF8;
using ::base::android::ConvertUTF8ToJavaString;
using ::base::android::JavaParamRef;
using ::base::android::ScopedJavaLocalRef;
using ::i18n::phonenumbers::PhoneNumber;
using ::i18n::phonenumbers::PhoneNumberUtil;

// Formats the |phone_number| to the specified |format|. Returns the original
// number if the operation is not possible.
std::string FormatPhoneNumber(const std::string& phone_number,
                              PhoneNumberUtil::PhoneNumberFormat format) {
  const std::string default_region_code =
      autofill::AutofillCountry::CountryCodeForLocale(
          g_browser_process->GetApplicationLocale());

  PhoneNumber parsed_number;
  PhoneNumberUtil* phone_number_util = PhoneNumberUtil::GetInstance();
  if (phone_number_util->Parse(phone_number, default_region_code,
                               &parsed_number) !=
      PhoneNumberUtil::NO_PARSING_ERROR) {
    return phone_number;
  }

  std::string formatted_number;
  phone_number_util->Format(parsed_number, format, &formatted_number);
  return formatted_number;
}

}  // namespace

// Formats the given number |jphone_number| to
// i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::INTERNATIONAL format
// by using i18n::phonenumbers::PhoneNumberUtil::Format.
ScopedJavaLocalRef<jstring> FormatForDisplay(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& jphone_number) {
  return ConvertUTF8ToJavaString(
      env,
      FormatPhoneNumber(ConvertJavaStringToUTF8(env, jphone_number),
                        PhoneNumberUtil::PhoneNumberFormat::INTERNATIONAL));
}

// Formats the given number |jphone_number| to
// i18n::phonenumbers::PhoneNumberUtil::PhoneNumberFormat::E164 format by using
// i18n::phonenumbers::PhoneNumberUtil::Format , as defined in the Payment
// Request spec
// (https://w3c.github.io/browser-payment-api/#paymentrequest-updated-algorithm)
ScopedJavaLocalRef<jstring> FormatForResponse(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jstring>& jphone_number) {
  return ConvertUTF8ToJavaString(
      env, FormatPhoneNumber(ConvertJavaStringToUTF8(env, jphone_number),
                             PhoneNumberUtil::PhoneNumberFormat::E164));
}

// Checks whether the given number |jphone_number| is valid by using
// i18n::phonenumbers::PhoneNumberUtil::IsValidNumber.
jboolean IsValidNumber(JNIEnv* env,
                       const base::android::JavaParamRef<jclass>& jcaller,
                       const JavaParamRef<jstring>& jphone_number) {
  const std::string phone_number = ConvertJavaStringToUTF8(env, jphone_number);
  const std::string default_region_code =
      autofill::AutofillCountry::CountryCodeForLocale(
          g_browser_process->GetApplicationLocale());

  PhoneNumber parsed_number;
  PhoneNumberUtil* phone_number_util = PhoneNumberUtil::GetInstance();
  if (phone_number_util->Parse(phone_number, default_region_code,
                               &parsed_number) !=
      PhoneNumberUtil::NO_PARSING_ERROR) {
    return false;
  }

  return phone_number_util->IsValidNumber(parsed_number);
}

}  // namespace autofill

// static
bool RegisterPhoneNumberUtil(JNIEnv* env) {
  return autofill::RegisterNativesImpl(env);
}

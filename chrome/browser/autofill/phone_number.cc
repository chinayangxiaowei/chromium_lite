// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/phone_number.h"

#include "base/basictypes.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autofill/autofill_profile.h"
#include "chrome/browser/autofill/autofill_type.h"
#include "chrome/browser/autofill/field_types.h"
#include "chrome/browser/autofill/phone_number_i18n.h"

namespace {

const char16 kPhoneNumberSeparators[] = { ' ', '.', '(', ')', '-', 0 };

// The number of digits in a phone number.
const size_t kPhoneNumberLength = 7;

// The number of digits in an area code.
const size_t kPhoneCityCodeLength = 3;

const AutofillType::FieldTypeSubGroup kAutofillPhoneTypes[] = {
  AutofillType::PHONE_NUMBER,
  AutofillType::PHONE_CITY_CODE,
  AutofillType::PHONE_COUNTRY_CODE,
  AutofillType::PHONE_CITY_AND_NUMBER,
  AutofillType::PHONE_WHOLE_NUMBER,
};

const int kAutofillPhoneLength = arraysize(kAutofillPhoneTypes);

void StripPunctuation(string16* number) {
  RemoveChars(*number, kPhoneNumberSeparators, number);
}

}  // namespace

PhoneNumber::PhoneNumber(AutofillProfile* profile)
    : phone_group_(AutofillType::NO_GROUP),
      profile_(profile) {
}

PhoneNumber::PhoneNumber(AutofillType::FieldTypeGroup phone_group,
                         AutofillProfile* profile)
    : phone_group_(phone_group),
      profile_(profile) {
}

PhoneNumber::PhoneNumber(const PhoneNumber& number) : FormGroup() {
  *this = number;
}

PhoneNumber::~PhoneNumber() {}

PhoneNumber& PhoneNumber::operator=(const PhoneNumber& number) {
  if (this == &number)
    return *this;
  phone_group_ = number.phone_group_;
  number_ = number.number_;
  profile_ = number.profile_;
  cached_parsed_phone_ = number.cached_parsed_phone_;
  return *this;
}

void PhoneNumber::GetSupportedTypes(FieldTypeSet* supported_types) const {
  supported_types->insert(GetWholeNumberType());
  supported_types->insert(GetNumberType());
  supported_types->insert(GetCityCodeType());
  supported_types->insert(GetCityAndNumberType());
  supported_types->insert(GetCountryCodeType());
}

string16 PhoneNumber::GetInfo(AutofillFieldType type) const {
  if (type == GetWholeNumberType())
    return number_;

  UpdateCacheIfNeeded();
  if (!cached_parsed_phone_.IsValidNumber())
    return string16();

  if (type == GetNumberType())
    return cached_parsed_phone_.GetNumber();

  if (type == GetCityCodeType())
    return cached_parsed_phone_.GetCityCode();

  if (type == GetCountryCodeType())
    return cached_parsed_phone_.GetCountryCode();

  if (type == GetCityAndNumberType()) {
    string16 city_and_local(cached_parsed_phone_.GetCityCode());
    city_and_local.append(cached_parsed_phone_.GetNumber());
    return city_and_local;
  }

  return string16();
}

void PhoneNumber::SetInfo(AutofillFieldType type, const string16& value) {
  // Store the group the first time we set some info.
  if (phone_group_ == AutofillType::NO_GROUP)
    phone_group_ = AutofillType(type).group();

  FieldTypeSubGroup subgroup = AutofillType(type).subgroup();
  if (subgroup != AutofillType::PHONE_CITY_AND_NUMBER &&
      subgroup != AutofillType::PHONE_WHOLE_NUMBER) {
    // Only full phone numbers should be set directly.  The remaining field
    // field types are read-only.
    return;
  }

  number_ = value;
  cached_parsed_phone_ = autofill_i18n::PhoneObject(number_, locale());
}

// Normalize phones if |type| is a whole number:
//   (650)2345678 -> 6502345678
//   1-800-FLOWERS -> 18003569377
// If the phone cannot be normalized, returns the stored value verbatim.
string16 PhoneNumber::GetCanonicalizedInfo(AutofillFieldType type) const {
  string16 phone = GetInfo(type);
  if (type != GetWholeNumberType())
    return phone;

  string16 normalized_phone = autofill_i18n::NormalizePhoneNumber(phone,
                                                                  locale());
  if (!normalized_phone.empty())
    return normalized_phone;

  return phone;
}

bool PhoneNumber::SetCanonicalizedInfo(AutofillFieldType type,
                                       const string16& value) {
  string16 number = value;
  StripPunctuation(&number);
  SetInfo(type, number);

  return NormalizePhone();
}

void PhoneNumber::GetMatchingTypes(const string16& text,
                                   FieldTypeSet* matching_types) const {
  string16 stripped_text = text;
  StripPunctuation(&stripped_text);
  FormGroup::GetMatchingTypes(stripped_text, matching_types);

  // For US numbers, also compare to the three-digit prefix and the four-digit
  // suffix, since websites often split numbers into these two fields.
  string16 number = GetCanonicalizedInfo(GetNumberType());
  if (locale() == "US" && number.size() == (kPrefixLength + kSuffixLength)) {
    string16 prefix = number.substr(kPrefixOffset, kPrefixLength);
    string16 suffix = number.substr(kSuffixOffset, kSuffixLength);
    if (text == prefix || text == suffix)
      matching_types->insert(GetNumberType());
  }

  string16 whole_number = GetCanonicalizedInfo(GetWholeNumberType());
  if (!whole_number.empty() &&
      autofill_i18n::NormalizePhoneNumber(text, locale()) == whole_number) {
    matching_types->insert(GetWholeNumberType());
  }
}

bool PhoneNumber::NormalizePhone() {
  // Empty number does not need normalization.
  if (number_.empty())
    return true;

  UpdateCacheIfNeeded();
  number_ = cached_parsed_phone_.GetWholeNumber();
  return !number_.empty();
}

std::string PhoneNumber::locale() const {
  if (!profile_) {
    NOTREACHED();
    return "US";
  }

  return profile_->CountryCode();
}

void PhoneNumber::UpdateCacheIfNeeded() const {
  if (!number_.empty() && cached_parsed_phone_.GetLocale() != locale())
    cached_parsed_phone_ = autofill_i18n::PhoneObject(number_, locale());
}

AutofillFieldType PhoneNumber::GetNumberType() const {
  if (phone_group_ == AutofillType::PHONE_HOME)
    return PHONE_HOME_NUMBER;
  else if (phone_group_ == AutofillType::PHONE_FAX)
    return PHONE_FAX_NUMBER;
  else
    NOTREACHED();
  return UNKNOWN_TYPE;
}

AutofillFieldType PhoneNumber::GetCityCodeType() const {
  if (phone_group_ == AutofillType::PHONE_HOME)
    return PHONE_HOME_CITY_CODE;
  else if (phone_group_ == AutofillType::PHONE_FAX)
    return PHONE_FAX_CITY_CODE;
  else
    NOTREACHED();
  return UNKNOWN_TYPE;
}

AutofillFieldType PhoneNumber::GetCountryCodeType() const {
  if (phone_group_ == AutofillType::PHONE_HOME)
    return PHONE_HOME_COUNTRY_CODE;
  else if (phone_group_ == AutofillType::PHONE_FAX)
    return PHONE_FAX_COUNTRY_CODE;
  else
    NOTREACHED();
  return UNKNOWN_TYPE;
}

AutofillFieldType PhoneNumber::GetCityAndNumberType() const {
  if (phone_group_ == AutofillType::PHONE_HOME)
    return PHONE_HOME_CITY_AND_NUMBER;
  else if (phone_group_ == AutofillType::PHONE_FAX)
    return PHONE_FAX_CITY_AND_NUMBER;
  else
    NOTREACHED();
  return UNKNOWN_TYPE;
}

AutofillFieldType PhoneNumber::GetWholeNumberType() const {
  if (phone_group_ == AutofillType::PHONE_HOME)
    return PHONE_HOME_WHOLE_NUMBER;
  else if (phone_group_ == AutofillType::PHONE_FAX)
    return PHONE_FAX_WHOLE_NUMBER;
  else
    NOTREACHED();
  return UNKNOWN_TYPE;
}

PhoneNumber::PhoneCombineHelper::PhoneCombineHelper(
    AutofillType::FieldTypeGroup phone_group)
    : phone_group_(phone_group) {
}

PhoneNumber::PhoneCombineHelper::~PhoneCombineHelper() {
}

bool PhoneNumber::PhoneCombineHelper::SetInfo(AutofillFieldType field_type,
                                              const string16& value) {
  PhoneNumber temp(phone_group_, NULL);

  if (field_type == temp.GetCountryCodeType()) {
    country_ = value;
    return true;
  }

  if (field_type == temp.GetCityCodeType()) {
    city_ = value;
    return true;
  }

  if (field_type == temp.GetCityAndNumberType()) {
    phone_ = value;
    return true;
  }

  if (field_type == temp.GetWholeNumberType()) {
    whole_number_ = value;
    return true;
  }

  if (field_type == temp.GetNumberType()) {
    phone_.append(value);
    return true;
  }

  return false;
}

bool PhoneNumber::PhoneCombineHelper::ParseNumber(const std::string& locale,
                                                  string16* value) {
  if (!whole_number_.empty()) {
    *value = whole_number_;
    return true;
  }

  return autofill_i18n::ConstructPhoneNumber(
      country_, city_, phone_,
      locale,
      (country_.empty() ?
          autofill_i18n::NATIONAL : autofill_i18n::INTERNATIONAL),
      value);
}

bool PhoneNumber::PhoneCombineHelper::IsEmpty() const {
  return phone_.empty() && whole_number_.empty();
}

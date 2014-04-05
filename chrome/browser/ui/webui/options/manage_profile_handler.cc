// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/manage_profile_handler.h"

#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "base/value_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/common/notification_service.h"
#include "grit/generated_resources.h"

ManageProfileHandler::ManageProfileHandler() {
}

ManageProfileHandler::~ManageProfileHandler() {
}

void ManageProfileHandler::GetLocalizedValues(
    DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  static OptionsStringResource resources[] = {
    { "manageProfilesTitle", IDS_PROFILES_MANAGE_TITLE },
    { "manageProfilesNameLabel", IDS_PROFILES_MANAGE_NAME_LABEL },
    { "manageProfilesDuplicateNameError",
        IDS_PROFILES_MANAGE_DUPLICATE_NAME_ERROR },
    { "manageProfilesIconLabel", IDS_PROFILES_MANAGE_ICON_LABEL },
    { "deleteProfileTitle", IDS_PROFILES_DELETE_TITLE },
    { "deleteProfileOK", IDS_PROFILES_DELETE_OK_BUTTON_LABEL },
    { "deleteProfileMessage", IDS_PROFILES_DELETE_MESSAGE },
  };

  RegisterStrings(localized_strings, resources, arraysize(resources));
}

void ManageProfileHandler::Initialize() {
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CACHED_INFO_CHANGED,
                 NotificationService::AllSources());
  InitializeDefaultProfileIcons();
  SendProfileNames();
}

void ManageProfileHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback("setProfileNameAndIcon",
      NewCallback(this, &ManageProfileHandler::SetProfileNameAndIcon));
  web_ui_->RegisterMessageCallback("deleteProfile",
      NewCallback(this, &ManageProfileHandler::DeleteProfile));
}

void ManageProfileHandler::Observe(int type,
                                    const NotificationSource& source,
                                    const NotificationDetails& details) {
  if (type == chrome::NOTIFICATION_PROFILE_CACHED_INFO_CHANGED)
    SendProfileNames();
  else
    OptionsPageUIHandler::Observe(type, source, details);
}

void ManageProfileHandler::InitializeDefaultProfileIcons() {
  ListValue image_url_list;
  for (size_t i = 0; i < ProfileInfoCache::GetDefaultAvatarIconCount(); i++) {
    std::string url = ProfileInfoCache::GetDefaultAvatarIconUrl(i);
    image_url_list.Append(Value::CreateStringValue(url));
  }

  web_ui_->CallJavascriptFunction(
      "ManageProfileOverlay.receiveDefaultProfileIcons",
      image_url_list);
}

void ManageProfileHandler::SendProfileNames() {
  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  DictionaryValue profile_name_dict;
  for (size_t i = 0, e = cache.GetNumberOfProfiles(); i < e; ++i)
    profile_name_dict.SetBoolean(UTF16ToUTF8(cache.GetNameOfProfileAtIndex(i)),
                                 true);

  web_ui_->CallJavascriptFunction("ManageProfileOverlay.receiveProfileNames",
                                  profile_name_dict);
}

void ManageProfileHandler::SetProfileNameAndIcon(const ListValue* args) {
  Value* file_path_value;
  FilePath profile_file_path;
  if (!args->Get(0, &file_path_value) ||
      !base::GetValueAsFilePath(*file_path_value, &profile_file_path))
    return;

  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  size_t profile_index = cache.GetIndexOfProfileWithPath(profile_file_path);
  if (profile_index == std::string::npos)
    return;

  string16 new_profile_name;
  if (!args->GetString(1, &new_profile_name))
    return;

  cache.SetNameOfProfileAtIndex(profile_index, new_profile_name);

  string16 icon_url;
  size_t new_icon_index;
  if (!args->GetString(2, &icon_url) ||
      !cache.IsDefaultAvatarIconUrl(UTF16ToUTF8(icon_url), &new_icon_index))
    return;

  cache.SetAvatarIconOfProfileAtIndex(profile_index, new_icon_index);
}

void ManageProfileHandler::DeleteProfile(const ListValue* args) {
  Value* file_path_value;
  FilePath profile_file_path;
  if (!args->Get(0, &file_path_value) ||
      !base::GetValueAsFilePath(*file_path_value, &profile_file_path))
    return;

  g_browser_process->profile_manager()->ScheduleProfileForDeletion(
      profile_file_path);
}


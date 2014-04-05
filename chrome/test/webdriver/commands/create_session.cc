// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/webdriver/commands/create_session.h"

#include <sstream>
#include <string>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_temp_dir.h"
#include "base/stringprintf.h"
#include "base/values.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/zip.h"
#include "chrome/test/webdriver/commands/response.h"
#include "chrome/test/webdriver/session.h"
#include "chrome/test/webdriver/session_manager.h"
#include "chrome/test/webdriver/webdriver_error.h"

namespace webdriver {

CreateSession::CreateSession(const std::vector<std::string>& path_segments,
                             const DictionaryValue* const parameters)
    : Command(path_segments, parameters) {}

CreateSession::~CreateSession() {}

bool CreateSession::DoesPost() { return true; }

void CreateSession::ExecutePost(Response* const response) {
  DictionaryValue *capabilities = NULL;
  if (!GetDictionaryParameter("desiredCapabilities", &capabilities)) {
    response->SetError(new Error(
        kBadRequest, "Missing or invalid 'desiredCapabilities'"));
    return;
  }

  CommandLine command_line_options(CommandLine::NO_PROGRAM);
  ListValue* switches = NULL;
  const char* kCustomSwitchesKey = "chrome.switches";
  if (capabilities->GetListWithoutPathExpansion(kCustomSwitchesKey,
                                                &switches)) {
    for (size_t i = 0; i < switches->GetSize(); ++i) {
      std::string switch_string;
      if (!switches->GetString(i, &switch_string)) {
        response->SetError(new Error(
            kBadRequest, "Custom switch is not a string"));
        return;
      }
      size_t separator_index = switch_string.find("=");
      if (separator_index != std::string::npos) {
        CommandLine::StringType switch_string_native;
        if (!switches->GetString(i, &switch_string_native)) {
          response->SetError(new Error(
              kBadRequest, "Custom switch is not a string"));
          return;
        }
        command_line_options.AppendSwitchNative(
            switch_string.substr(0, separator_index),
            switch_string_native.substr(separator_index + 1));
      } else {
        command_line_options.AppendSwitch(switch_string);
      }
    }
  } else if (capabilities->HasKey(kCustomSwitchesKey)) {
    response->SetError(new Error(
        kBadRequest, "Custom switches must be a list"));
    return;
  }
  Value* verbose_value;
  if (capabilities->GetWithoutPathExpansion("chrome.verbose", &verbose_value)) {
    bool verbose;
    if (verbose_value->GetAsBoolean(&verbose) && verbose) {
      // Since logging is shared among sessions, if any session requests verbose
      // logging, verbose logging will be enabled for all sessions. It is not
      // possible to turn it off.
      logging::SetMinLogLevel(logging::LOG_INFO);
    } else {
      response->SetError(new Error(
          kBadRequest, "verbose must be a boolean true or false"));
      return;
    }
  }

  FilePath browser_exe;
  FilePath::StringType path;
  if (capabilities->GetStringWithoutPathExpansion("chrome.binary", &path))
    browser_exe = FilePath(path);

  ScopedTempDir temp_dir;
  FilePath temp_user_data_dir;

  std::string base64_profile;
  if (capabilities->GetStringWithoutPathExpansion("chrome.profile",
                                                  &base64_profile)) {
    if (!temp_dir.CreateUniqueTempDir()) {
      response->SetError(new Error(
          kUnknownError, "Could not create temporary directory."));
      return;
    }

    std::string data;
    if (!base::Base64Decode(base64_profile, &data)) {
      response->SetError(new Error(
          kBadRequest, "Invalid base64 encoded user profile"));
      return;
    }

    FilePath temp_profile_zip = temp_dir.path().AppendASCII("profile.zip");
    if (!file_util::WriteFile(temp_profile_zip, data.c_str(), data.length())) {
      response->SetError(new Error(
          kUnknownError, "Could not write temporary profile zip file."));
      return;
    }

    temp_user_data_dir = temp_dir.path().AppendASCII("user_data_dir");
    if (!Unzip(temp_profile_zip, temp_user_data_dir)) {
      response->SetError(new Error(
          kBadRequest, "Could not unarchive provided user profile"));
      return;
    }
  }

  // Session manages its own liftime, so do not call delete.
  Session* session = new Session();
  Error* error = session->Init(browser_exe,
                               temp_user_data_dir,
                               command_line_options);
  if (error) {
    response->SetError(error);
    return;
  }

  bool native_events_required = false;
  Value* native_events_value = NULL;
  if (capabilities->GetWithoutPathExpansion(
          "chrome.nativeEvents", &native_events_value)) {
    if (native_events_value->GetAsBoolean(&native_events_required)) {
      session->set_use_native_events(native_events_required);
    }
  }
  bool screenshot_on_error = false;
  if (capabilities->GetBoolean(
          "takeScreenshotOnError", &screenshot_on_error)) {
    session->set_screenshot_on_error(screenshot_on_error);
  }

  LOG(INFO) << "Created session " << session->id();
  // Redirect to a relative URI. Although prohibited by the HTTP standard,
  // this is what the IEDriver does. Finding the actual IP address is
  // difficult, and returning the hostname causes perf problems with the python
  // bindings on Windows.
  std::ostringstream stream;
  stream << SessionManager::GetInstance()->url_base() << "/session/"
         << session->id();
  response->SetStatus(kSeeOther);
  response->SetValue(Value::CreateStringValue(stream.str()));
}

}  // namespace webdriver

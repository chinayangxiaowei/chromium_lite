// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_settings_storage_unittest.h"

namespace {

void Param(
    ExtensionSettings* settings,
    const std::string& extension_id,
    const ExtensionSettings::Callback& callback) {
  settings->GetStorageForTesting(
      ExtensionSettingsStorage::LEVELDB,
      false,
      extension_id,
      callback);
}

}  // namespace

INSTANTIATE_TEST_CASE_P(
    ExtensionSettingsLeveldbStorage,
    ExtensionSettingsStorageTest,
    testing::Values(&Param));

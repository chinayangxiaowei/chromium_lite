// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility_util.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_accessibility_api.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "grit/browser_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace chromeos {
namespace accessibility {


void EnableAccessibility(bool enabled) {
  bool accessibility_enabled = g_browser_process &&
      g_browser_process->local_state()->GetBoolean(
          prefs::kAccessibilityEnabled);
  if (accessibility_enabled == enabled) {
    return;
  }

  g_browser_process->local_state()->SetBoolean(
      prefs::kAccessibilityEnabled, enabled);
  // Explicitly call SavePersistentPrefs instead of ScheduleSavePersistentPrefs
  // so that this change gets written immediately, in case the user shuts
  // down right now.  TODO(dmazzoni) switch this back to
  // ScheduleSavePersistentPrefs once http://crosbug.com/19491 is fixed.
  g_browser_process->local_state()->SavePersistentPrefs();
  ExtensionAccessibilityEventRouter::GetInstance()->
      SetAccessibilityEnabled(enabled);

  // Load/Unload ChromeVox
  Profile* profile = ProfileManager::GetDefaultProfile();
  ExtensionService* extension_service =
      profile->GetExtensionService();
  std::string manifest = ResourceBundle::GetSharedInstance().
      GetRawDataResource(IDR_CHROMEVOX_MANIFEST).as_string();
  FilePath path = FilePath(extension_misc::kAccessExtensionPath)
      .AppendASCII(extension_misc::kChromeVoxDirectoryName);
  ExtensionService::ComponentExtensionInfo info(manifest, path);
  if (enabled) { // Load ChromeVox
    extension_service->register_component_extension(info);
    extension_service->LoadComponentExtension(info);
  } else { // Unload ChromeVox
    extension_service->UnloadComponentExtension(info);
    extension_service->UnregisterComponentExtension(info);
  }
}

void ToggleAccessibility() {
  bool accessibility_enabled = g_browser_process &&
      g_browser_process->local_state()->GetBoolean(
          prefs::kAccessibilityEnabled);
  accessibility_enabled = !accessibility_enabled;
  EnableAccessibility(accessibility_enabled);
};

}  // namespace accessibility
}  // namespace chromeos

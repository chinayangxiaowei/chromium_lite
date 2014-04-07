// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/management_policy.h"

namespace extensions {

namespace {

void GetExtensionNameAndId(const Extension* extension,
                           std::string* name,
                           std::string* id) {
  // The extension may be NULL in testing.
  *id = extension ? extension->id() : "[test]";
  *name = extension ? extension->name() : "test";
}

}  // namespace

ManagementPolicy::ManagementPolicy() {
}

ManagementPolicy::~ManagementPolicy() {
}

bool ManagementPolicy::Provider::UserMayLoad(const Extension* extension,
                                             string16* error) const {
  return true;
}

bool ManagementPolicy::Provider::UserMayModifySettings(
    const Extension* extension, string16* error) const {
  return true;
}

bool ManagementPolicy::Provider::MustRemainEnabled(const Extension* extension,
                                                   string16* error) const {
  return false;
}

bool ManagementPolicy::Provider::MustRemainDisabled(
    const Extension* extension,
    Extension::DisableReason* reason,
    string16* error) const {
  return false;
}

void ManagementPolicy::RegisterProvider(Provider* provider) {
  providers_.insert(provider);
}

void ManagementPolicy::UnregisterProvider(Provider* provider) {
  providers_.erase(provider);
}

bool ManagementPolicy::UserMayLoad(const Extension* extension,
                                   string16* error) const {
  return ApplyToProviderList(&Provider::UserMayLoad, "Installation",
                             true, extension, error);
}

bool ManagementPolicy::UserMayModifySettings(const Extension* extension,
                                             string16* error) const {
  return ApplyToProviderList(&Provider::UserMayModifySettings, "Modification",
                             true, extension, error);
}

bool ManagementPolicy::MustRemainEnabled(const Extension* extension,
                                         string16* error) const {
  return ApplyToProviderList(&Provider::MustRemainEnabled, "Disabling",
                             false, extension, error);
}

bool ManagementPolicy::MustRemainDisabled(const Extension* extension,
                                          Extension::DisableReason* reason,
                                          string16* error) const {
  for (ProviderList::const_iterator it = providers_.begin();
       it != providers_.end(); ++it)
    if ((*it)->MustRemainDisabled(extension, reason, error))
      return true;

  return false;
}

void ManagementPolicy::UnregisterAllProviders() {
  providers_.clear();
}

int ManagementPolicy::GetNumProviders() const {
  return providers_.size();
}

bool ManagementPolicy::ApplyToProviderList(ProviderFunction function,
                                           const char* debug_operation_name,
                                           bool normal_result,
                                           const Extension* extension,
                                           string16* error) const {
  for (ProviderList::const_iterator it = providers_.begin();
       it != providers_.end(); ++it) {
    const Provider* provider = *it;
    bool result = (provider->*function)(extension, error);
    if (result != normal_result) {
      std::string id;
      std::string name;
      GetExtensionNameAndId(extension, &name, &id);
      DVLOG(1) << debug_operation_name << " of extension " << name
               << " (" << id << ")"
               << " prohibited by " << provider->GetDebugPolicyProviderName();
      return !normal_result;
    }
  }
  return normal_result;
}

}  // namespace extensions

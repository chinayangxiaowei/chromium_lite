// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/configuration_policy_reader.h"

#include <map>
#include <vector>

#include "base/stl_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/browser_policy_connector.h"
#include "chrome/browser/policy/configuration_policy_pref_store.h"
#include "chrome/browser/policy/configuration_policy_store_interface.h"

namespace policy {

// This class functions as a container for policy status information used by the
// ConfigurationPolicyReader class. It obtains policy values from a
// ConfigurationPolicyProvider and maps them to their status information.
class ConfigurationPolicyStatusKeeper
    : public ConfigurationPolicyStoreInterface {
 public:
  ConfigurationPolicyStatusKeeper(ConfigurationPolicyProvider* provider,
                                  PolicyStatusInfo::PolicyLevel policy_level);
  virtual ~ConfigurationPolicyStatusKeeper();

  // ConfigurationPolicyStoreInterface methods.
  virtual void Apply(ConfigurationPolicyType policy, base::Value* value);

  // Returns a pointer to a DictionaryValue containing the status information
  // of the policy |policy|. The caller acquires ownership of the returned
  // value. Returns NULL if no such policy is stored in this keeper.
  DictionaryValue* GetPolicyStatus(ConfigurationPolicyType policy) const;

 private:
  typedef std::map<ConfigurationPolicyType, PolicyStatusInfo*> PolicyStatusMap;
  typedef std::map<ConfigurationPolicyType, string16> PolicyNameMap;
  typedef ConfigurationPolicyProvider::PolicyDefinitionList
      PolicyDefinitionList;

  // Mapping from ConfigurationPolicyType to PolicyStatusInfo.
  PolicyStatusMap policy_map_;

  // The level of the policies stored in this keeper (mandatory or
  // recommended).
  PolicyStatusInfo::PolicyLevel policy_level_;

  DISALLOW_COPY_AND_ASSIGN(ConfigurationPolicyStatusKeeper);
};

// ConfigurationPolicyStatusKeeper
ConfigurationPolicyStatusKeeper::ConfigurationPolicyStatusKeeper(
    ConfigurationPolicyProvider* provider,
    PolicyStatusInfo::PolicyLevel policy_level) : policy_level_(policy_level) {

  if (!provider->Provide(this))
    LOG(WARNING) << "Failed to get policy from provider.";
}

ConfigurationPolicyStatusKeeper::~ConfigurationPolicyStatusKeeper() {
  STLDeleteContainerPairSecondPointers(policy_map_.begin(), policy_map_.end());
  policy_map_.clear();
}

void ConfigurationPolicyStatusKeeper::Apply(
    ConfigurationPolicyType policy, base::Value* value) {
  string16 name = PolicyStatus::GetPolicyName(policy);

  if (name == string16()) {
    NOTREACHED();
  }

  // TODO(simo) actually determine whether the policy is a user or a device one
  // and whether the policy could be enforced or not once this information
  // is available.
  PolicyStatusInfo* info = new PolicyStatusInfo(name,
                                                PolicyStatusInfo::USER,
                                                policy_level_,
                                                value,
                                                PolicyStatusInfo::ENFORCED,
                                                string16());
  policy_map_[policy] = info;
}

DictionaryValue* ConfigurationPolicyStatusKeeper::GetPolicyStatus(
    ConfigurationPolicyType policy) const {
  PolicyStatusMap::const_iterator entry = policy_map_.find(policy);
  return entry != policy_map_.end() ?
      (entry->second)->GetDictionaryValue() : NULL;
}

// ConfigurationPolicyReader
ConfigurationPolicyReader::~ConfigurationPolicyReader() {
}

void ConfigurationPolicyReader::OnUpdatePolicy() {
  Refresh();
}

void ConfigurationPolicyReader::OnProviderGoingAway() {
  provider_ = NULL;
}

// static
ConfigurationPolicyReader*
    ConfigurationPolicyReader::CreateManagedPlatformPolicyReader() {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  return new ConfigurationPolicyReader(
    connector->GetManagedPlatformProvider(), PolicyStatusInfo::MANDATORY);
}

// static
ConfigurationPolicyReader*
    ConfigurationPolicyReader::CreateManagedCloudPolicyReader() {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  return new ConfigurationPolicyReader(
      connector->GetManagedCloudProvider(), PolicyStatusInfo::MANDATORY);
}

// static
ConfigurationPolicyReader*
    ConfigurationPolicyReader::CreateRecommendedPlatformPolicyReader() {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  return new ConfigurationPolicyReader(
      connector->GetRecommendedPlatformProvider(),
          PolicyStatusInfo::RECOMMENDED);
}

// static
ConfigurationPolicyReader*
    ConfigurationPolicyReader::CreateRecommendedCloudPolicyReader() {
  BrowserPolicyConnector* connector =
      g_browser_process->browser_policy_connector();
  return new ConfigurationPolicyReader(
      connector->GetRecommendedCloudProvider(), PolicyStatusInfo::RECOMMENDED);
}

DictionaryValue* ConfigurationPolicyReader::GetPolicyStatus(
    ConfigurationPolicyType policy) const {
  return policy_keeper_->GetPolicyStatus(policy);
}

ConfigurationPolicyReader::ConfigurationPolicyReader(
    ConfigurationPolicyProvider* provider,
    PolicyStatusInfo::PolicyLevel policy_level)
    : provider_(provider),
      policy_level_(policy_level) {
  if (provider_) {
    // Read initial policy.
    policy_keeper_.reset(
        new ConfigurationPolicyStatusKeeper(provider, policy_level));
    registrar_.Init(provider_, this);
  }
}

ConfigurationPolicyReader::ConfigurationPolicyReader()
    : provider_(NULL),
      policy_level_(PolicyStatusInfo::LEVEL_UNDEFINED),
      policy_keeper_(NULL) {
}

void ConfigurationPolicyReader::Refresh() {
  if (!provider_)
    return;

  // Read policy state into a new keeper and swap out old keeper.
  scoped_ptr<ConfigurationPolicyStatusKeeper> new_keeper(
      new ConfigurationPolicyStatusKeeper(provider_, policy_level_));
  policy_keeper_.reset(new_keeper.release());
}

// PolicyStatus
PolicyStatus::PolicyStatus(ConfigurationPolicyReader* managed_platform,
                           ConfigurationPolicyReader* managed_cloud,
                           ConfigurationPolicyReader* recommended_platform,
                           ConfigurationPolicyReader* recommended_cloud)
    : managed_platform_(managed_platform),
      managed_cloud_(managed_cloud),
      recommended_platform_(recommended_platform),
      recommended_cloud_(recommended_cloud) {
}

PolicyStatus::~PolicyStatus() {
}

ListValue* PolicyStatus::GetPolicyStatusList(bool* any_policies_sent) const {
  ListValue* result = new ListValue();
  std::vector<DictionaryValue*> unsent_policies;

  *any_policies_sent = false;
  const PolicyDefinitionList* supported_policies =
      ConfigurationPolicyPrefStore::GetChromePolicyDefinitionList();
  const PolicyDefinitionList::Entry* policy = supported_policies->begin;
  for ( ; policy != supported_policies->end; ++policy) {
    if (!AddPolicyFromReaders(policy->policy_type, result)) {
      PolicyStatusInfo info(ASCIIToUTF16(policy->name),
                            PolicyStatusInfo::SOURCE_TYPE_UNDEFINED,
                            PolicyStatusInfo::LEVEL_UNDEFINED,
                            Value::CreateNullValue(),
                            PolicyStatusInfo::STATUS_UNDEFINED,
                            string16());
      unsent_policies.push_back(info.GetDictionaryValue());
    } else {
      *any_policies_sent = true;
    }
  }

  // Add policies that weren't actually sent from providers to list.
  std::vector<DictionaryValue*>::const_iterator info = unsent_policies.begin();
  for ( ; info != unsent_policies.end(); ++info)
    result->Append(*info);

  return result;
}

// static
string16 PolicyStatus::GetPolicyName(ConfigurationPolicyType policy_type) {
  static std::map<ConfigurationPolicyType, string16> name_map;
  static const ConfigurationPolicyProvider::PolicyDefinitionList*
      supported_policies;

  if (!supported_policies) {
    supported_policies =
        ConfigurationPolicyPrefStore::GetChromePolicyDefinitionList();

    // Create mapping from ConfigurationPolicyTypes to actual policy names.
    const ConfigurationPolicyProvider::PolicyDefinitionList::Entry* entry =
        supported_policies->begin;
    for ( ; entry != supported_policies->end; ++entry)
      name_map[entry->policy_type] = ASCIIToUTF16(entry->name);
  }

  std::map<ConfigurationPolicyType, string16>::const_iterator entry =
      name_map.find(policy_type);

  if (entry == name_map.end())
    return string16();

  return entry->second;
}

bool PolicyStatus::AddPolicyFromReaders(
    ConfigurationPolicyType policy, ListValue* list) const {
  DictionaryValue* mp_policy =
      managed_platform_->GetPolicyStatus(policy);
  DictionaryValue* mc_policy =
      managed_cloud_->GetPolicyStatus(policy);
  DictionaryValue* rp_policy =
      recommended_platform_->GetPolicyStatus(policy);
  DictionaryValue* rc_policy =
      recommended_cloud_->GetPolicyStatus(policy);

  bool added_policy = mp_policy || mc_policy || rp_policy || rc_policy;

  if (mp_policy)
    list->Append(mp_policy);
  if (mc_policy)
    list->Append(mc_policy);
  if (rp_policy)
    list->Append(rp_policy);
  if (rc_policy)
    list->Append(rc_policy);

  return added_policy;
}

} // namespace policy

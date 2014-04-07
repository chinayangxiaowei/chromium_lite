// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/cloud/cloud_policy_core.h"

#include "base/basictypes.h"
#include "base/message_loop/message_loop.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/testing_pref_service.h"
#include "chrome/browser/policy/cloud/cloud_policy_constants.h"
#include "chrome/browser/policy/cloud/cloud_policy_refresh_scheduler.h"
#include "chrome/browser/policy/cloud/mock_cloud_policy_client.h"
#include "chrome/browser/policy/cloud/mock_cloud_policy_store.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class CloudPolicyCoreTest : public testing::Test,
                            public CloudPolicyCore::Observer {
 protected:
  CloudPolicyCoreTest()
      : core_(PolicyNamespaceKey(dm_protocol::kChromeUserPolicyType,
                                 std::string()),
              &store_,
              loop_.message_loop_proxy()),
        core_connected_callback_count_(0),
        refresh_scheduler_started_callback_count_(0),
        core_disconnecting_callback_count_(0),
        bad_callback_count_(0) {
    prefs_.registry()->RegisterIntegerPref(
        policy_prefs::kUserPolicyRefreshRate,
        CloudPolicyRefreshScheduler::kDefaultRefreshDelayMs);
    core_.AddObserver(this);
  }

  virtual ~CloudPolicyCoreTest() {
    core_.RemoveObserver(this);
  }

  virtual void OnCoreConnected(CloudPolicyCore* core) OVERRIDE {
    // Make sure core is connected at callback time.
    if (core_.client())
      core_connected_callback_count_++;
    else
      bad_callback_count_++;
  }

  virtual void OnRefreshSchedulerStarted(CloudPolicyCore* core) OVERRIDE {
    // Make sure refresh scheduler is started at callback time.
    if (core_.refresh_scheduler())
      refresh_scheduler_started_callback_count_++;
    else
      bad_callback_count_++;
  }

  virtual void OnCoreDisconnecting(CloudPolicyCore* core) OVERRIDE {
    // Make sure core is still connected at callback time.
    if (core_.client())
      core_disconnecting_callback_count_++;
    else
      bad_callback_count_++;
  }

  base::MessageLoop loop_;

  TestingPrefServiceSimple prefs_;
  MockCloudPolicyStore store_;
  CloudPolicyCore core_;

  int core_connected_callback_count_;
  int refresh_scheduler_started_callback_count_;
  int core_disconnecting_callback_count_;
  int bad_callback_count_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CloudPolicyCoreTest);
};

TEST_F(CloudPolicyCoreTest, ConnectAndDisconnect) {
  EXPECT_TRUE(core_.store());
  EXPECT_FALSE(core_.client());
  EXPECT_FALSE(core_.service());
  EXPECT_FALSE(core_.refresh_scheduler());

  // Connect() brings up client and service.
  core_.Connect(scoped_ptr<CloudPolicyClient>(new MockCloudPolicyClient()));
  EXPECT_TRUE(core_.client());
  EXPECT_TRUE(core_.service());
  EXPECT_FALSE(core_.refresh_scheduler());
  EXPECT_EQ(1, core_connected_callback_count_);
  EXPECT_EQ(0, refresh_scheduler_started_callback_count_);
  EXPECT_EQ(0, core_disconnecting_callback_count_);

  // Disconnect() goes back to no client and service.
  core_.Disconnect();
  EXPECT_FALSE(core_.client());
  EXPECT_FALSE(core_.service());
  EXPECT_FALSE(core_.refresh_scheduler());
  EXPECT_EQ(1, core_connected_callback_count_);
  EXPECT_EQ(0, refresh_scheduler_started_callback_count_);
  EXPECT_EQ(1, core_disconnecting_callback_count_);

  // Calling Disconnect() twice doesn't do bad things.
  core_.Disconnect();
  EXPECT_FALSE(core_.client());
  EXPECT_FALSE(core_.service());
  EXPECT_FALSE(core_.refresh_scheduler());
  EXPECT_EQ(1, core_connected_callback_count_);
  EXPECT_EQ(0, refresh_scheduler_started_callback_count_);
  EXPECT_EQ(1, core_disconnecting_callback_count_);
  EXPECT_EQ(0, bad_callback_count_);
}

TEST_F(CloudPolicyCoreTest, RefreshScheduler) {
  EXPECT_FALSE(core_.refresh_scheduler());
  core_.Connect(scoped_ptr<CloudPolicyClient>(new MockCloudPolicyClient()));
  core_.StartRefreshScheduler();
  ASSERT_TRUE(core_.refresh_scheduler());

  int default_refresh_delay = core_.refresh_scheduler()->refresh_delay();

  const int kRefreshRate = 1000 * 60 * 60;
  prefs_.SetInteger(policy_prefs::kUserPolicyRefreshRate, kRefreshRate);
  core_.TrackRefreshDelayPref(&prefs_, policy_prefs::kUserPolicyRefreshRate);
  EXPECT_EQ(kRefreshRate, core_.refresh_scheduler()->refresh_delay());

  prefs_.ClearPref(policy_prefs::kUserPolicyRefreshRate);
  EXPECT_EQ(default_refresh_delay, core_.refresh_scheduler()->refresh_delay());

  EXPECT_EQ(1, core_connected_callback_count_);
  EXPECT_EQ(1, refresh_scheduler_started_callback_count_);
  EXPECT_EQ(0, core_disconnecting_callback_count_);
  EXPECT_EQ(0, bad_callback_count_);
}

}  // namespace policy

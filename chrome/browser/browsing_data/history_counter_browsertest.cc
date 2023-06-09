// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browsing_data/core/counters/history_counter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/web_history_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/web_history_service.h"
#include "components/history/core/test/fake_web_history_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace {

using browsing_data::BrowsingDataCounter;
using browsing_data::HistoryCounter;

class HistoryCounterTest : public InProcessBrowserTest {
 public:
  HistoryCounterTest() {}
  ~HistoryCounterTest() override {};

  void SetUpOnMainThread() override {
    time_ = base::Time::Now();
    history_service_ = HistoryServiceFactory::GetForProfileWithoutCreating(
        browser()->profile());
    fake_web_history_service_.reset(new history::FakeWebHistoryService(
        ProfileOAuth2TokenServiceFactory::GetForProfile(browser()->profile()),
        SigninManagerFactory::GetForProfile(browser()->profile()),
        browser()->profile()->GetRequestContext()));

    SetHistoryDeletionPref(true);
    SetDeletionPeriodPref(browsing_data::TimePeriod::ALL_TIME);
  }

  void AddVisit(const std::string url) {
    history_service_->AddPage(GURL(url), time_, history::SOURCE_BROWSED);
  }

  const base::Time& GetCurrentTime() {
    return time_;
  }

  void RevertTimeInDays(int days) {
    time_ -= base::TimeDelta::FromDays(days);
  }

  void SetHistoryDeletionPref(bool value) {
    browser()->profile()->GetPrefs()->SetBoolean(
        browsing_data::prefs::kDeleteBrowsingHistory, value);
  }

  void SetDeletionPeriodPref(browsing_data::TimePeriod period) {
    browser()->profile()->GetPrefs()->SetInteger(
        browsing_data::prefs::kDeleteTimePeriod, static_cast<int>(period));
  }

  void WaitForCounting() {
    run_loop_.reset(new base::RunLoop());
    run_loop_->Run();
  }

  BrowsingDataCounter::ResultInt GetLocalResult() {
    DCHECK(finished_);
    return local_result_;
  }

  bool HasSyncedVisits() {
    DCHECK(finished_);
    return has_synced_visits_;
  }

  void Callback(std::unique_ptr<BrowsingDataCounter::Result> result) {
    finished_ = result->Finished();

    if (finished_) {
      auto* history_result =
          static_cast<HistoryCounter::HistoryResult*>(result.get());

      local_result_ = history_result->Value();
      has_synced_visits_ = history_result->has_synced_visits();
    }

    if (run_loop_ && finished_)
      run_loop_->Quit();
  }

  history::WebHistoryService* GetFakeWebHistoryService(Profile* profile,
                                                       bool check_sync_status) {
    // |check_sync_status| is true when the factory should check if
    // history sync is enabled.
    if (!check_sync_status ||
        WebHistoryServiceFactory::GetForProfile(profile)) {
      return fake_web_history_service_.get();
    }
    return nullptr;
  }

  history::WebHistoryService* GetRealWebHistoryService(Profile* profile) {
    return WebHistoryServiceFactory::GetForProfile(profile);
  }

  history::HistoryService* GetHistoryService() { return history_service_; }

 private:
  std::unique_ptr<base::RunLoop> run_loop_;
  history::HistoryService* history_service_;
  std::unique_ptr<history::FakeWebHistoryService> fake_web_history_service_;
  base::Time time_;

  bool finished_;
  BrowsingDataCounter::ResultInt local_result_;
  bool has_synced_visits_;
};

// Tests that the counter considers duplicate visits from the same day
// to be a single item.
IN_PROC_BROWSER_TEST_F(HistoryCounterTest, DuplicateVisits) {
  AddVisit("https://www.google.com");   // 1 item
  AddVisit("https://www.google.com");
  AddVisit("https://www.chrome.com");   // 2 items
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.example.com");  // 3 items

  RevertTimeInDays(1);
  AddVisit("https://www.google.com");   // 4 items
  AddVisit("https://www.example.com");  // 5 items
  AddVisit("https://www.example.com");

  RevertTimeInDays(1);
  AddVisit("https://www.chrome.com");   // 6 items
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.google.com");   // 7 items
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.google.com");
  AddVisit("https://www.google.com");
  AddVisit("https://www.chrome.com");

  Profile* profile = browser()->profile();

  HistoryCounter counter(
      GetHistoryService(),
      base::Bind(&HistoryCounterTest::GetRealWebHistoryService,
                 base::Unretained(this), base::Unretained(profile)),
      ProfileSyncServiceFactory::GetForProfile(profile));

  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&HistoryCounterTest::Callback, base::Unretained(this)));
  counter.Restart();

  WaitForCounting();
  EXPECT_EQ(7u, GetLocalResult());
}

// Tests that the counter starts counting automatically when the deletion
// pref changes to true.
IN_PROC_BROWSER_TEST_F(HistoryCounterTest, PrefChanged) {
  SetHistoryDeletionPref(false);
  AddVisit("https://www.google.com");
  AddVisit("https://www.chrome.com");

  Profile* profile = browser()->profile();

  HistoryCounter counter(
      GetHistoryService(),
      base::Bind(&HistoryCounterTest::GetRealWebHistoryService,
                 base::Unretained(this), base::Unretained(profile)),
      ProfileSyncServiceFactory::GetForProfile(profile));

  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&HistoryCounterTest::Callback, base::Unretained(this)));
  SetHistoryDeletionPref(true);

  WaitForCounting();
  EXPECT_EQ(2u, GetLocalResult());
}

// Tests that changing the deletion period restarts the counting, and that
// the result takes visit dates into account.
IN_PROC_BROWSER_TEST_F(HistoryCounterTest, PeriodChanged) {
  AddVisit("https://www.google.com");

  RevertTimeInDays(2);
  AddVisit("https://www.google.com");
  AddVisit("https://www.example.com");

  RevertTimeInDays(4);
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.example.com");

  RevertTimeInDays(20);
  AddVisit("https://www.google.com");
  AddVisit("https://www.chrome.com");
  AddVisit("https://www.example.com");

  RevertTimeInDays(10);
  AddVisit("https://www.example.com");
  AddVisit("https://www.example.com");
  AddVisit("https://www.example.com");

  Profile* profile = browser()->profile();

  HistoryCounter counter(
      GetHistoryService(),
      base::Bind(&HistoryCounterTest::GetRealWebHistoryService,
                 base::Unretained(this), base::Unretained(profile)),
      ProfileSyncServiceFactory::GetForProfile(profile));

  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&HistoryCounterTest::Callback, base::Unretained(this)));

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  WaitForCounting();
  EXPECT_EQ(1u, GetLocalResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_DAY);
  WaitForCounting();
  EXPECT_EQ(1u, GetLocalResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_WEEK);
  WaitForCounting();
  EXPECT_EQ(5u, GetLocalResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::FOUR_WEEKS);
  WaitForCounting();
  EXPECT_EQ(8u, GetLocalResult());

  SetDeletionPeriodPref(browsing_data::TimePeriod::ALL_TIME);
  WaitForCounting();
  EXPECT_EQ(9u, GetLocalResult());
}

// Test the behavior for a profile that syncs history.
IN_PROC_BROWSER_TEST_F(HistoryCounterTest, Synced) {
  // WebHistoryService makes network requests, so we need to use a fake one
  // for testing.
  Profile* profile = browser()->profile();

  HistoryCounter counter(
      GetHistoryService(),
      base::Bind(&HistoryCounterTest::GetFakeWebHistoryService,
                 base::Unretained(this), base::Unretained(profile), false),
      ProfileSyncServiceFactory::GetForProfile(profile));

  counter.Init(
      profile->GetPrefs(), browsing_data::ClearBrowsingDataTab::ADVANCED,
      base::Bind(&HistoryCounterTest::Callback, base::Unretained(this)));

  history::FakeWebHistoryService* service =
    static_cast<history::FakeWebHistoryService*>(GetFakeWebHistoryService(
        profile, false));

  // No entries locally and no entries in Sync.
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(0u, GetLocalResult());
  EXPECT_FALSE(HasSyncedVisits());

  // No entries locally. There are some entries in Sync, but they are out of the
  // time range.
  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  service->AddSyncedVisit(
      "www.google.com", GetCurrentTime() - base::TimeDelta::FromHours(2));
  service->AddSyncedVisit(
      "www.chrome.com", GetCurrentTime() - base::TimeDelta::FromHours(2));
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(0u, GetLocalResult());
  EXPECT_FALSE(HasSyncedVisits());

  // No entries locally, but some entries in Sync.
  service->AddSyncedVisit("www.google.com", GetCurrentTime());
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(0u, GetLocalResult());
  EXPECT_TRUE(HasSyncedVisits());

  // To err on the safe side, if the server request fails, we assume that there
  // might be some items on the server.
  service->SetupFakeResponse(true /* success */,
                                              net::HTTP_INTERNAL_SERVER_ERROR);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(0u, GetLocalResult());
  EXPECT_TRUE(HasSyncedVisits());

  // Same when the entire query fails.
  service->SetupFakeResponse(false /* success */,
                                              net::HTTP_INTERNAL_SERVER_ERROR);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(0u, GetLocalResult());
  EXPECT_TRUE(HasSyncedVisits());

  // Nonzero local count, nonempty sync.
  AddVisit("https://www.google.com");
  AddVisit("https://www.chrome.com");
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(2u, GetLocalResult());
  EXPECT_TRUE(HasSyncedVisits());

  // Nonzero local count, empty sync.
  service->ClearSyncedVisits();
  service->SetupFakeResponse(true /* success */, net::HTTP_OK);
  counter.Restart();
  WaitForCounting();
  EXPECT_EQ(2u, GetLocalResult());
  EXPECT_FALSE(HasSyncedVisits());
}

}  // namespace

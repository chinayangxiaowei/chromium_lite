// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/profile_sync_service_harness.h"
#include "chrome/test/live_sync/live_bookmarks_sync_test.h"
#include "chrome/test/live_sync/performance/sync_timing_helper.h"

static const int kNumBookmarks = 150;

// TODO(braffert): Consider the range / resolution of these test points.
static const int kNumBenchmarkPoints = 18;
static const int kBenchmarkPoints[] = {1, 10, 20, 30, 40, 50, 75, 100, 125,
                                       150, 175, 200, 225, 250, 300, 350, 400,
                                       500};

// TODO(braffert): Move this class into its own .h/.cc files.  What should the
// class files be named as opposed to the file containing the tests themselves?
class BookmarksSyncPerfTest
    : public TwoClientLiveBookmarksSyncTest {
 public:
  BookmarksSyncPerfTest() : url_number(0), url_title_number(0) {}

  // Adds |num_urls| new unique bookmarks to the bookmark bar for |profile|.
  void AddURLs(int profile, int num_urls);

  // Updates the URL for all bookmarks in the bookmark bar for |profile|.
  void UpdateURLs(int profile);

  // Removes all bookmarks in the bookmark bar for |profile|.
  void RemoveURLs(int profile);

  // Remvoes all bookmarks in the bookmark bars for all profiles.  Called
  // between benchmark iterations.
  void Cleanup();

 private:
  // Returns a new unique bookmark URL.
  std::string NextIndexedURL();

  // Returns a new unique bookmark title.
  std::wstring NextIndexedURLTitle();

  int url_number;
  int url_title_number;
  DISALLOW_COPY_AND_ASSIGN(BookmarksSyncPerfTest);
};

void BookmarksSyncPerfTest::AddURLs(int profile, int num_urls) {
  for (int i = 0; i < num_urls; ++i) {
    ASSERT_TRUE(AddURL(
        profile, 0, NextIndexedURLTitle(), GURL(NextIndexedURL())) != NULL);
  }
}

void BookmarksSyncPerfTest::UpdateURLs(int profile) {
  for (int i = 0; i < GetBookmarkBarNode(profile)->child_count(); ++i) {
    ASSERT_TRUE(SetURL(profile, GetBookmarkBarNode(profile)->GetChild(i),
                       GURL(NextIndexedURL())));
  }
}

void BookmarksSyncPerfTest::RemoveURLs(int profile) {
  while (GetBookmarkBarNode(profile)->child_count()) {
    Remove(profile, GetBookmarkBarNode(profile), 0);
  }
}

void BookmarksSyncPerfTest::Cleanup() {
  for (int i = 0; i < num_clients(); ++i) {
    RemoveURLs(i);
  }
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_EQ(0, GetBookmarkBarNode(0)->child_count());
  ASSERT_TRUE(AllModelsMatch());
}

std::string BookmarksSyncPerfTest::NextIndexedURL() {
  return IndexedURL(url_number++);
}

std::wstring BookmarksSyncPerfTest::NextIndexedURLTitle() {
  return IndexedURLTitle(url_title_number++);
}

// TCM ID - 7556828.
IN_PROC_BROWSER_TEST_F(BookmarksSyncPerfTest, Add) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  AddURLs(0, kNumBookmarks);
  base::TimeDelta dt =
      SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumBookmarks, GetBookmarkBarNode(0)->child_count());
  ASSERT_TRUE(AllModelsMatch());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

// TCM ID - 7564762.
IN_PROC_BROWSER_TEST_F(BookmarksSyncPerfTest, Update) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  AddURLs(0, kNumBookmarks);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  UpdateURLs(0);
  base::TimeDelta dt =
      SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(kNumBookmarks, GetBookmarkBarNode(0)->child_count());
  ASSERT_TRUE(AllModelsMatch());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

// TCM ID - 7566626.
IN_PROC_BROWSER_TEST_F(BookmarksSyncPerfTest, Delete) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  AddURLs(0, kNumBookmarks);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  RemoveURLs(0);
  base::TimeDelta dt =
      SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(0, GetBookmarkBarNode(0)->child_count());
  ASSERT_TRUE(AllModelsMatch());

  // TODO(braffert): Compare timings against some target value.
  VLOG(0) << std::endl << "dt: " << dt.InSecondsF() << " s";
}

IN_PROC_BROWSER_TEST_F(BookmarksSyncPerfTest, DISABLED_Benchmark) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  for (int i = 0; i < kNumBenchmarkPoints; ++i) {
    int num_bookmarks = kBenchmarkPoints[i];
    AddURLs(0, num_bookmarks);
    base::TimeDelta dt_add =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    ASSERT_EQ(num_bookmarks, GetBookmarkBarNode(0)->child_count());
    ASSERT_TRUE(AllModelsMatch());
    VLOG(0) << std::endl << "Add: " << num_bookmarks << " "
            << dt_add.InSecondsF();

    UpdateURLs(0);
    base::TimeDelta dt_update =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    ASSERT_EQ(num_bookmarks, GetBookmarkBarNode(0)->child_count());
    ASSERT_TRUE(AllModelsMatch());
    VLOG(0) << std::endl << "Update: " << num_bookmarks << " "
            << dt_update.InSecondsF();

    RemoveURLs(0);
    base::TimeDelta dt_delete =
        SyncTimingHelper::TimeMutualSyncCycle(GetClient(0), GetClient(1));
    ASSERT_EQ(0, GetBookmarkBarNode(0)->child_count());
    ASSERT_TRUE(AllModelsMatch());
    VLOG(0) << std::endl << "Delete: " << num_bookmarks << " "
            << dt_delete.InSecondsF();

    Cleanup();
  }
}

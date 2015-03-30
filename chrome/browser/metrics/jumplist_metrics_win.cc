// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/jumplist_metrics_win.h"

#include "base/metrics/histogram.h"

namespace jumplist {

const char kMostVisitedCategory[] = "most-visited";
const char kProfilesCategory[] = "profiles";
const char kRecentlyClosedCategory[] = "recently-closed";

void LogJumplistActionFromSwitchValue(const std::string& value) {
  JumplistCategory metric = CATEGORY_UNKNOWN;
  if (value == kMostVisitedCategory)
    metric = MOST_VISITED_URL;
  else if (value == kProfilesCategory)
    metric = SWITCH_TO_PROFILE;
  else if (value == kRecentlyClosedCategory)
    metric = RECENTLY_CLOSED_URL;
  DCHECK_NE(metric, CATEGORY_UNKNOWN);

  UMA_HISTOGRAM_ENUMERATION(
      "WinJumplist.Action", metric, NUM_JUMPLIST_CATEGORY_METRICS);
}

}  // namespace jumplist

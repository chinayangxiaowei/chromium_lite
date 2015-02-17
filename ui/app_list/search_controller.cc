// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/search_controller.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/app_list/search/history.h"
#include "ui/app_list/search_box_model.h"
#include "ui/app_list/search_provider.h"
#include "ui/app_list/search_result.h"

namespace {

// Maximum time (in milliseconds) to wait to the search providers to finish.
const int kStopTimeMS = 1500;
}

namespace app_list {

SearchController::SearchController(SearchBoxModel* search_box,
                                   AppListModel::SearchResults* results,
                                   History* history)
  : search_box_(search_box),
    dispatching_query_(false),
    mixer_(new Mixer(results)),
    history_(history) {
  mixer_->Init();
}

SearchController::~SearchController() {
}

void SearchController::Start() {
  Stop();

  base::string16 query;
  base::TrimWhitespace(search_box_->text(), base::TRIM_ALL, &query);

  dispatching_query_ = true;
  for (Providers::iterator it = providers_.begin();
       it != providers_.end();
       ++it) {
    (*it)->Start(query);
  }
  dispatching_query_ = false;

  OnResultsChanged();

  stop_timer_.Start(FROM_HERE,
                    base::TimeDelta::FromMilliseconds(kStopTimeMS),
                    base::Bind(&SearchController::Stop,
                               base::Unretained(this)));
}

void SearchController::Stop() {
  stop_timer_.Stop();

  for (Providers::iterator it = providers_.begin();
       it != providers_.end();
       ++it) {
    (*it)->Stop();
  }
}

void SearchController::OpenResult(SearchResult* result, int event_flags) {
  // Count AppList.Search here because it is composed of search + action.
  base::RecordAction(base::UserMetricsAction("AppList_Search"));

  result->Open(event_flags);

  if (history_ && history_->IsReady()) {
    history_->AddLaunchEvent(base::UTF16ToUTF8(search_box_->text()),
                             result->id());
  }
}

void SearchController::InvokeResultAction(SearchResult* result,
                                          int action_index,
                                          int event_flags) {
  // TODO(xiyuan): Hook up with user learning.
  result->InvokeAction(action_index, event_flags);
}

void SearchController::AddProvider(Mixer::GroupId group,
                                   scoped_ptr<SearchProvider> provider) {
  provider->set_result_changed_callback(base::Bind(
      &SearchController::OnResultsChanged,
      base::Unretained(this)));
  mixer_->AddProviderToGroup(group, provider.get());
  providers_.push_back(provider.release());  // Takes ownership.
}

void SearchController::OnResultsChanged() {
  if (dispatching_query_)
    return;

  KnownResults known_results;
  if (history_ && history_->IsReady()) {
    history_->GetKnownResults(base::UTF16ToUTF8(search_box_->text()))
        ->swap(known_results);
  }

  mixer_->MixAndPublish(known_results);
}

}  // namespace app_list

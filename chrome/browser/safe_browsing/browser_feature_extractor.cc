// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/browser_feature_extractor.h"

#include <map>
#include <utility>

#include "base/stl_util.h"
#include "base/stringprintf.h"
#include "base/string_util.h"
#include "base/task.h"
#include "base/time.h"
#include "chrome/common/safe_browsing/csd.pb.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/browser_features.h"
#include "chrome/browser/safe_browsing/client_side_detection_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_util.h"
#include "content/common/page_transition_types.h"
#include "content/browser/browser_thread.h"
#include "content/browser/cancelable_request.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "crypto/sha2.h"
#include "googleurl/src/gurl.h"

namespace safe_browsing {

const int BrowserFeatureExtractor::kHashPrefixLength = 5;

BrowseInfo::BrowseInfo() {}

BrowseInfo::~BrowseInfo() {}

static void AddFeature(const std::string& feature_name,
                       double feature_value,
                       ClientPhishingRequest* request) {
  DCHECK(request);
  ClientPhishingRequest::Feature* feature =
      request->add_non_model_feature_map();
  feature->set_name(feature_name);
  feature->set_value(feature_value);
  VLOG(2) << "Browser feature: " << feature->name() << " " << feature->value();
}

static void AddNavigationFeatures(const std::string& feature_prefix,
                                  const NavigationController& controller,
                                  int index,
                                  ClientPhishingRequest* request) {
  NavigationEntry* entry = controller.GetEntryAtIndex(index);
  bool is_secure_referrer = entry->referrer().SchemeIsSecure();
  if (!is_secure_referrer) {
    AddFeature(StringPrintf("%s%s=%s",
                            feature_prefix.c_str(),
                            features::kReferrer,
                            entry->referrer().spec().c_str()),
               1.0,
               request);
  }
  AddFeature(feature_prefix + features::kHasSSLReferrer,
             is_secure_referrer ? 1.0 : 0.0,
             request);
  AddFeature(feature_prefix + features::kPageTransitionType,
             static_cast<double>(
                 PageTransition::StripQualifier(entry->transition_type())),
             request);
  AddFeature(feature_prefix + features::kIsFirstNavigation,
             index == 0 ? 1.0 : 0.0,
             request);
}

static void PossiblyAddRedirectNavigationFeatures(
    const std::string& feature_prefix,
    const NavigationController& controller,
    int index,
    ClientPhishingRequest* request) {
  NavigationEntry* entry = controller.GetEntryAtIndex(index);
  // Add additional features for the start of the redirect chain, if this entry
  // is part of one and the chain starts on a different page.
  if (PageTransition::IsRedirect(entry->transition_type()) &&
      (entry->transition_type() & PageTransition::CHAIN_START) == 0) {
    for (index--; index >= 0; index--) {
      entry = controller.GetEntryAtIndex(index);
      if (entry->transition_type() & PageTransition::CHAIN_START) {
        AddNavigationFeatures(feature_prefix + features::kRedirectPrefix,
                              controller,
                              index,
                              request);
        return;
      }
    }
  }
}

BrowserFeatureExtractor::BrowserFeatureExtractor(
    TabContents* tab,
    ClientSideDetectionService* service)
    : tab_(tab),
      service_(service),
      ALLOW_THIS_IN_INITIALIZER_LIST(method_factory_(this)) {
  DCHECK(tab);
}

BrowserFeatureExtractor::~BrowserFeatureExtractor() {
  method_factory_.RevokeAll();
  // Delete all the pending extractions (delete callback and request objects).
  STLDeleteContainerPairPointers(pending_extractions_.begin(),
                                 pending_extractions_.end());
  // Also cancel all the pending history service queries.
  HistoryService* history;
  bool success = GetHistoryService(&history);
  DCHECK(success || pending_queries_.size() == 0);
  // Cancel all the pending history lookups and cleanup the memory.
  for (PendingQueriesMap::iterator it = pending_queries_.begin();
       it != pending_queries_.end(); ++it) {
    if (history) {
      history->CancelRequest(it->first);
    }
    ExtractionData& extraction = it->second;
    delete extraction.first;  // delete request
    delete extraction.second;  // delete callback
  }
  pending_queries_.clear();
}

void BrowserFeatureExtractor::ExtractFeatures(const BrowseInfo* info,
                                              ClientPhishingRequest* request,
                                              DoneCallback* callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(request);
  DCHECK(info);
  DCHECK_EQ(0U, request->url().find("http:"));
  DCHECK(callback);
  if (!callback) {
    DLOG(ERROR) << "ExtractFeatures called without a callback object";
    return;
  }

  // Extract features pertaining to this navigation.
  const NavigationController& controller = tab_->controller();
  int url_index = -1;
  int first_host_index = -1;

  GURL request_url(request->url());
  int index = controller.GetCurrentEntryIndex();
  // The url that we are extracting features for should already be commited.
  DCHECK_NE(index, -1);
  for (; index >= 0; index--) {
    NavigationEntry* entry = controller.GetEntryAtIndex(index);
    if (url_index == -1 && entry->url() == request_url) {
      // It's possible that we've been on the on the possibly phishy url before
      // in this tab, so make sure that we use the latest navigation for
      // features.
      // Note that it's possible that the url_index should always be the
      // latest entry, but I'm worried about possible races during a navigation
      // and transient entries (i.e. interstiatials) so for now we will just
      // be cautious.
      url_index = index;
    } else if (index < url_index) {
      if (entry->url().host() == request_url.host()) {
        first_host_index = index;
      } else {
        // We have found the possibly phishing url, but we are no longer on the
        // host. No reason to look back any further.
        break;
      }
    }
  }

  // Add features pertaining to how we got to
  //   1) The candidate url
  //   2) The redirect leading to the candidate url (assuming there was one).
  //   3) The first url on the same host as the candidate url (assuming that
  //      it's different from the candidate url).
  //   4) The redirect leading to the first url on the host (assuming there was
  //      one).
  if (url_index != -1) {
    AddNavigationFeatures("", controller, url_index, request);
    PossiblyAddRedirectNavigationFeatures("", controller, url_index, request);
  }
  if (first_host_index != -1) {
    AddNavigationFeatures(features::kHostPrefix,
                          controller,
                          first_host_index,
                          request);
    PossiblyAddRedirectNavigationFeatures(features::kHostPrefix,
                                          controller,
                                          first_host_index,
                                          request);
  }

  ExtractBrowseInfoFeatures(*info, request);
  ComputeURLHash(request);
  pending_extractions_.insert(std::make_pair(request, callback));
  MessageLoop::current()->PostTask(
      FROM_HERE,
      method_factory_.NewRunnableMethod(
          &BrowserFeatureExtractor::StartExtractFeatures,
          request, callback));
}

void BrowserFeatureExtractor::ExtractBrowseInfoFeatures(
    const BrowseInfo& info,
    ClientPhishingRequest* request) {
  if (service_) {
    for (std::set<std::string>::const_iterator it = info.ips.begin();
         it != info.ips.end(); ++it) {
      if (service_->IsBadIpAddress(*it)) {
        AddFeature(features::kBadIpFetch + *it, 1.0, request);
      }
    }
  }
  if (info.unsafe_resource.get()) {
    // A SafeBrowsing interstitial was shown for the current URL.
    AddFeature(features::kSafeBrowsingMaliciousUrl +
               info.unsafe_resource->url.spec(),
               1.0,
               request);
    AddFeature(features::kSafeBrowsingOriginalUrl +
               info.unsafe_resource->original_url.spec(),
               1.0,
               request);
    AddFeature(features::kSafeBrowsingIsSubresource,
               info.unsafe_resource->is_subresource ? 1.0 : 0.0,
               request);
    AddFeature(features::kSafeBrowsingThreatType,
               static_cast<double>(info.unsafe_resource->threat_type),
               request);
  }

}

void BrowserFeatureExtractor::StartExtractFeatures(
    ClientPhishingRequest* request,
    DoneCallback* callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ExtractionData extraction = std::make_pair(request, callback);
  size_t removed = pending_extractions_.erase(extraction);
  DCHECK_EQ(1U, removed);
  HistoryService* history;
  if (!request || !request->IsInitialized() || !GetHistoryService(&history)) {
    callback->Run(false, request);
    delete callback;
    return;
  }
  CancelableRequestProvider::Handle handle = history->QueryURL(
      GURL(request->url()),
      true /* wants_visits */,
      &request_consumer_,
      NewCallback(this,
                  &BrowserFeatureExtractor::QueryUrlHistoryDone));

  StorePendingQuery(handle, request, callback);
}

void BrowserFeatureExtractor::QueryUrlHistoryDone(
    CancelableRequestProvider::Handle handle,
    bool success,
    const history::URLRow* row,
    history::VisitVector* visits) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ClientPhishingRequest* request;
  DoneCallback* callback;
  if (!GetPendingQuery(handle, &request, &callback)) {
    DLOG(FATAL) << "No pending history query found";
    return;
  }
  DCHECK(request);
  DCHECK(callback);
  if (!success) {
    // URL is not found in the history.  In practice this should not
    // happen (unless there is a real error) because we just visited
    // that URL.
    callback->Run(false, request);
    delete callback;
    return;
  }
  AddFeature(features::kUrlHistoryVisitCount,
             static_cast<double>(row->visit_count()),
             request);

  base::Time threshold = base::Time::Now() - base::TimeDelta::FromDays(1);
  int num_visits_24h_ago = 0;
  int num_visits_typed = 0;
  int num_visits_link = 0;
  for (history::VisitVector::const_iterator it = visits->begin();
       it != visits->end(); ++it) {
    if (!PageTransition::IsMainFrame(it->transition)) {
      continue;
    }
    if (it->visit_time < threshold) {
      ++num_visits_24h_ago;
    }
    PageTransition::Type transition = PageTransition::StripQualifier(
        it->transition);
    if (transition == PageTransition::TYPED) {
      ++num_visits_typed;
    } else if (transition == PageTransition::LINK) {
      ++num_visits_link;
    }
  }
  AddFeature(features::kUrlHistoryVisitCountMoreThan24hAgo,
             static_cast<double>(num_visits_24h_ago),
             request);
  AddFeature(features::kUrlHistoryTypedCount,
             static_cast<double>(num_visits_typed),
             request);
  AddFeature(features::kUrlHistoryLinkCount,
             static_cast<double>(num_visits_link),
             request);

  // Issue next history lookup for host visits.
  HistoryService* history;
  if (!GetHistoryService(&history)) {
    callback->Run(false, request);
    delete callback;
    return;
  }
  CancelableRequestProvider::Handle next_handle =
      history->GetVisibleVisitCountToHost(
          GURL(request->url()),
          &request_consumer_,
          NewCallback(this, &BrowserFeatureExtractor::QueryHttpHostVisitsDone));
  StorePendingQuery(next_handle, request, callback);
}

void BrowserFeatureExtractor::QueryHttpHostVisitsDone(
    CancelableRequestProvider::Handle handle,
    bool success,
    int num_visits,
    base::Time first_visit) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ClientPhishingRequest* request;
  DoneCallback* callback;
  if (!GetPendingQuery(handle, &request, &callback)) {
    DLOG(FATAL) << "No pending history query found";
    return;
  }
  DCHECK(request);
  DCHECK(callback);
  if (!success) {
    callback->Run(false, request);
    delete callback;
    return;
  }
  SetHostVisitsFeatures(num_visits, first_visit, true, request);

  // Same lookup but for the HTTPS URL.
  HistoryService* history;
  if (!GetHistoryService(&history)) {
    callback->Run(false, request);
    delete callback;
    return;
  }
  std::string https_url = request->url();
  CancelableRequestProvider::Handle next_handle =
      history->GetVisibleVisitCountToHost(
          GURL(https_url.replace(0, 5, "https:")),
          &request_consumer_,
          NewCallback(this,
                      &BrowserFeatureExtractor::QueryHttpsHostVisitsDone));
  StorePendingQuery(next_handle, request, callback);
}

void BrowserFeatureExtractor::QueryHttpsHostVisitsDone(
    CancelableRequestProvider::Handle handle,
    bool success,
    int num_visits,
    base::Time first_visit) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ClientPhishingRequest* request;
  DoneCallback* callback;
  if (!GetPendingQuery(handle, &request, &callback)) {
    DLOG(FATAL) << "No pending history query found";
    return;
  }
  DCHECK(request);
  DCHECK(callback);
  if (!success) {
    callback->Run(false, request);
    delete callback;
    return;
  }
  SetHostVisitsFeatures(num_visits, first_visit, false, request);
  callback->Run(true, request);  // We're done with all the history lookups.
  delete callback;
}

void BrowserFeatureExtractor::SetHostVisitsFeatures(
    int num_visits,
    base::Time first_visit,
    bool is_http_query,
    ClientPhishingRequest* request) {
  DCHECK(request);
  AddFeature(is_http_query ?
             features::kHttpHostVisitCount : features::kHttpsHostVisitCount,
             static_cast<double>(num_visits),
             request);
  if (num_visits > 0) {
    AddFeature(
        is_http_query ?
        features::kFirstHttpHostVisitMoreThan24hAgo :
        features::kFirstHttpsHostVisitMoreThan24hAgo,
        (first_visit < (base::Time::Now() - base::TimeDelta::FromDays(1))) ?
        1.0 : 0.0,
        request);
  }
}

void BrowserFeatureExtractor::StorePendingQuery(
    CancelableRequestProvider::Handle handle,
    ClientPhishingRequest* request,
    DoneCallback* callback) {
  DCHECK_EQ(0U, pending_queries_.count(handle));
  pending_queries_[handle] = std::make_pair(request, callback);
}

bool BrowserFeatureExtractor::GetPendingQuery(
    CancelableRequestProvider::Handle handle,
    ClientPhishingRequest** request,
    DoneCallback** callback) {
  PendingQueriesMap::iterator it = pending_queries_.find(handle);
  DCHECK(it != pending_queries_.end());
  if (it != pending_queries_.end()) {
    *request = it->second.first;
    *callback = it->second.second;
    pending_queries_.erase(it);
    return true;
  }
  return false;
}

bool BrowserFeatureExtractor::GetHistoryService(HistoryService** history) {
  *history = NULL;
  if (tab_ && tab_->browser_context()) {
    Profile* profile = Profile::FromBrowserContext(tab_->browser_context());
    *history = profile->GetHistoryService(Profile::EXPLICIT_ACCESS);
    if (*history) {
      return true;
    }
  }
  VLOG(2) << "Unable to query history.  No history service available.";
  return false;
}

void BrowserFeatureExtractor::ComputeURLHash(
    ClientPhishingRequest* request) {
  // Put the url into SafeBrowsing host suffix / path prefix format, with
  // query parameters stripped.
  std::string host, path, query;
  safe_browsing_util::CanonicalizeUrl(GURL(request->url()),
                                      &host, &path, &query);
  DCHECK(!host.empty()) << request->url();
  DCHECK(!path.empty()) << request->url();

  // Lowercase the URL.  Note: canonicalization converts the URL to ASCII.
  // Percent encoded characters will not be lowercased but this is consistent
  // with what we're doing on the server side.
  StringToLowerASCII(&host);
  StringToLowerASCII(&path);

  // Remove leading 'www.' from the host.
  if (host.size() > 4 && host.substr(0, 4) == "www.") {
    host.erase(0, 4);
  }
  // Remove everything after the last '/' to broaden the pattern.
  if (path.size() > 1 && *(path.rbegin()) != '/') {
    // The pattern never ends in foo.com/test? because we stripped CGI params.
    // Remove everything that comes after the last '/'.
    size_t last_path = path.rfind("/");
    path.erase(last_path + 1);
  }

  request->set_hash_prefix(crypto::SHA256HashString(host + path).substr(
      0, kHashPrefixLength));
}

};  // namespace safe_browsing

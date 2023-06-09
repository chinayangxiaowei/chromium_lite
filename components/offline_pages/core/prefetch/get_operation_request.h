// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_GET_OPERATION_REQUEST_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_GET_OPERATION_REQUEST_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace net {
class URLRequestContextGetter;
}

namespace offline_pages {

class PrefetchRequestFetcher;

// Sends this request to find out the current state of an operation that is
// triggered by GeneratePageBundleRequest but not finished at that time.
class GetOperationRequest {
 public:
  // |name| identifies the operation triggered by the GeneratePageBundleRequest.
  // It is retrieved from the operation data returned in the
  // GeneratePageBundleRequest response.
  GetOperationRequest(const std::string& name,
                      net::URLRequestContextGetter* request_context_getter,
                      const PrefetchRequestFinishedCallback& callback);
  ~GetOperationRequest();

 private:
  void OnCompleted(PrefetchRequestStatus status, const std::string& data);

  PrefetchRequestFinishedCallback callback_;
  std::unique_ptr<PrefetchRequestFetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(GetOperationRequest);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_GET_OPERATION_REQUEST_H_

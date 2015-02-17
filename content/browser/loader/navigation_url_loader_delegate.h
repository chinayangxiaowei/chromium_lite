// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_NAVIGATION_URL_LOADER_DELEGATE_H_
#define CONTENT_BROWSER_LOADER_NAVIGATION_URL_LOADER_DELEGATE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"

namespace net {
struct RedirectInfo;
}

namespace content {

class StreamHandle;
struct ResourceResponse;

// PlzNavigate: The delegate interface to NavigationURLLoader.
class CONTENT_EXPORT NavigationURLLoaderDelegate {
 public:
  // Called when the request is redirected. Call FollowRedirect to continue
  // processing the request.
  virtual void OnRequestRedirected(
      const net::RedirectInfo& redirect_info,
      const scoped_refptr<ResourceResponse>& response) = 0;

  // Called when the request receives its response. No further calls will be
  // made to the delegate. The response body is returned as a stream in
  // |body_stream|.
  virtual void OnResponseStarted(
      const scoped_refptr<ResourceResponse>& response,
      scoped_ptr<StreamHandle> body_stream) = 0;

  // Called if the request fails before receving a response. |net_error| is a
  // network error code for the failure.
  virtual void OnRequestFailed(int net_error) = 0;

 protected:
  NavigationURLLoaderDelegate() {}
  virtual ~NavigationURLLoaderDelegate() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationURLLoaderDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_NAVIGATION_URL_LOADER_DELEGATE_H_

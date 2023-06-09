// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_ANCESTOR_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_ANCESTOR_THROTTLE_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"

namespace net {
class HttpResponseHeaders;
}

namespace content {
class NavigationHandle;

// An AncestorThrottle is responsible for enforcing a resource's embedding
// rules, and blocking requests which violate them.
class CONTENT_EXPORT AncestorThrottle : public NavigationThrottle {
 public:
  enum class HeaderDisposition {
    NONE = 0,
    DENY,
    SAMEORIGIN,
    ALLOWALL,
    INVALID,
    CONFLICT,
    BYPASS
  };

  static std::unique_ptr<NavigationThrottle> MaybeCreateThrottleFor(
      NavigationHandle* handle);

  ~AncestorThrottle() override;

  NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  NavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;
  NavigationThrottle::ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AncestorThrottleTest, ParsingXFrameOptions);
  FRIEND_TEST_ALL_PREFIXES(AncestorThrottleTest, ErrorsParsingXFrameOptions);
  FRIEND_TEST_ALL_PREFIXES(AncestorThrottleTest,
                           IgnoreWhenFrameAncestorsPresent);

  explicit AncestorThrottle(NavigationHandle* handle);
  void ParseError(const std::string& value, HeaderDisposition disposition);
  void ConsoleError(HeaderDisposition disposition);

  // Parses an 'X-Frame-Options' header. If the result is either CONFLICT
  // or INVALID, |header_value| will be populated with the value which caused
  // the parse error.
  HeaderDisposition ParseHeader(const net::HttpResponseHeaders* headers,
                                std::string* header_value);

  NavigationThrottle::ThrottleCheckResult CheckContentSecurityPolicyFrameSrc(
      bool is_redirect);

  DISALLOW_COPY_AND_ASSIGN(AncestorThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_ANCESTOR_THROTTLE_H_

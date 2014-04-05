// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A complete set of unit tests for GaiaOAuthFetcher.
// Originally ported from GaiaAuthFetcher tests.

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "chrome/browser/net/chrome_cookie_notification_details.h"
#include "chrome/browser/net/gaia/gaia_oauth_consumer.h"
#include "chrome/browser/net/gaia/gaia_oauth_fetcher.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/net/gaia/google_service_auth_error.h"
#include "chrome/common/net/http_return.h"
#include "chrome/test/testing_profile.h"
#include "content/common/notification_service.h"
#include "content/common/test_url_fetcher_factory.h"
#include "content/common/url_fetcher.h"
#include "googleurl/src/gurl.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

// These constants are internal to the GaiaOAuthFetcher implementation, and
// should not be referenced except by unit tests, where they are required.
//
// The values must exactly match those defined in gaia_oauth_fetcher.cc
static const char kGetOAuthTokenUrl[] =
    "https://www.google.com/accounts/o8/GetOAuthToken";

static const char kOAuthGetAccessTokenUrl[] =
    "https://www.google.com/accounts/OAuthGetAccessToken";

static const char kOAuthWrapBridgeUrl[] =
    "https://www.google.com/accounts/OAuthWrapBridge";

static const char kOAuthWrapBridgeUserInfoScope[] =
    "https://www.googleapis.com/auth/userinfo.email";

static const char kOAuth1LoginScope[] =
    "https://www.google.com/accounts/OAuthLogin";

static const char kUserInfoUrl[] =
    "https://www.googleapis.com/oauth2/v1/userinfo";

class MockGaiaOAuthConsumer : public GaiaOAuthConsumer {
 public:
  MockGaiaOAuthConsumer() {}
  ~MockGaiaOAuthConsumer() {}

  MOCK_METHOD1(OnGetOAuthTokenSuccess, void(const std::string& oauth_token));
  MOCK_METHOD0(OnGetOAuthTokenFailure, void());

  MOCK_METHOD2(OnOAuthGetAccessTokenSuccess, void(const std::string& token,
                                                  const std::string& secret));
  MOCK_METHOD1(OnOAuthGetAccessTokenFailure,
               void(const GoogleServiceAuthError& error));

  MOCK_METHOD2(OnOAuthWrapBridgeSuccess,
               void(const std::string& token,
                    const std::string& expires_in));
  MOCK_METHOD1(OnOAuthWrapBridgeFailure,
               void(const GoogleServiceAuthError& error));

  MOCK_METHOD1(OnUserInfoSuccess, void(const std::string& email));
  MOCK_METHOD1(OnUserInfoFailure, void(const GoogleServiceAuthError& error));
};

class MockGaiaOAuthFetcher : public GaiaOAuthFetcher {
 public:
  MockGaiaOAuthFetcher(GaiaOAuthConsumer* consumer,
                       net::URLRequestContextGetter* getter,
                       Profile* profile,
                       const std::string& service_scope)
      : GaiaOAuthFetcher(consumer, getter, profile, service_scope) {}

  ~MockGaiaOAuthFetcher() {}

  MOCK_METHOD1(StartOAuthGetAccessToken,
               void(const std::string& oauth1_request_token));

  MOCK_METHOD4(StartOAuthWrapBridge,
               void(const std::string& oauth1_access_token,
                    const std::string& oauth1_access_token_secret,
                    const std::string& wrap_token_duration,
                    const std::string& oauth2_scope));

  MOCK_METHOD1(StartUserInfo, void(const std::string& oauth2_access_token));
};

#if 0  // Suppressing for now
TEST(GaiaOAuthFetcherTest, GetOAuthToken) {
  const std::string oauth_token = "4/OAuth1-Request_Token-1234567";
  base::Time creation = base::Time::Now();
  base::Time expiration = base::Time::Time();

  scoped_ptr<net::CookieMonster::CanonicalCookie> canonical_cookie;
  canonical_cookie.reset(
      new net::CookieMonster::CanonicalCookie(
          GURL("http://www.google.com/"),  // url
          "oauth_token",                   // name
          oauth_token,                     // value
          "www.google.com",                // domain
          "/accounts/o8/GetOAuthToken",    // path
          "",                              // mac_key
          "",                              // mac_algorithm
          creation,                        // creation
          expiration,                      // expiration
          creation,                        // last_access
          true,                            // secure
          true,                            // httponly
          false));                         // has_expires

  scoped_ptr<ChromeCookieDetails::ChromeCookieDetails> cookie_details;
  cookie_details.reset(
      new ChromeCookieDetails::ChromeCookieDetails(
          canonical_cookie.get(),
          false,
          net::CookieMonster::Delegate::CHANGE_COOKIE_EXPLICIT));

  MockGaiaOAuthConsumer consumer;
  EXPECT_CALL(consumer, OnGetOAuthTokenSuccess(oauth_token)).Times(1);

  TestingProfile profile;

  MockGaiaOAuthFetcher auth(&consumer,
                            profile.GetRequestContext(),
                            &profile,
                            std::string());
  EXPECT_CALL(auth, StartOAuthGetAccessToken(oauth_token)).Times(1);

  auth.Observe(
      chrome::NOTIFICATION_COOKIE_CHANGED,
      Source<Profile>(&profile),
      Details<ChromeCookieDetails>(cookie_details.get()));
}
#endif  // 0  // Suppressing for now


TEST(GaiaOAuthFetcherTest, OAuthGetAccessToken) {
  const std::string oauth_token="1/OAuth1-Access_Token-1234567890abcdefghijklm";
  const std::string oauth_token_secret="Dont_tell_the_secret-123";
  const std::string data("oauth_token="
                         "1%2FOAuth1-Access_Token-1234567890abcdefghijklm"
                         "&oauth_token_secret=Dont_tell_the_secret-123");

  MockGaiaOAuthConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuthGetAccessTokenSuccess(oauth_token,
                                           oauth_token_secret)).Times(1);

  TestingProfile profile;
  MockGaiaOAuthFetcher auth(&consumer,
                            profile .GetRequestContext(),
                            &profile,
                            std::string());
  EXPECT_CALL(auth, StartOAuthWrapBridge(oauth_token,
                                         oauth_token_secret,
                                         "3600",
                                         _)).Times(1);

  net::ResponseCookies cookies;
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  auth.OnURLFetchComplete(NULL,
                          GURL(kOAuthGetAccessTokenUrl),
                          status,
                          RC_REQUEST_OK,
                          cookies,
                          data);
}

TEST(GaiaOAuthFetcherTest, OAuthWrapBridge) {
  const std::string wrap_token="1/OAuth2-Access_Token-nopqrstuvwxyz1234567890";
  const std::string expires_in="3600";

  const std::string data("wrap_access_token="
                         "1%2FOAuth2-Access_Token-nopqrstuvwxyz1234567890"
                         "&wrap_access_token_expires_in=3600");

  MockGaiaOAuthConsumer consumer;
  EXPECT_CALL(consumer,
              OnOAuthWrapBridgeSuccess(wrap_token,
                                       expires_in)).Times(1);

  TestingProfile profile;
  MockGaiaOAuthFetcher auth(&consumer,
                            profile .GetRequestContext(),
                            &profile,
                            std::string());
  EXPECT_CALL(auth, StartUserInfo(wrap_token)).Times(1);

  net::ResponseCookies cookies;
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  auth.OnURLFetchComplete(NULL,
                          GURL(kOAuthWrapBridgeUrl),
                          status,
                          RC_REQUEST_OK,
                          cookies,
                          data);
}

TEST(GaiaOAuthFetcherTest, UserInfo) {
  const std::string email_address="someone@somewhere.net";
  const std::string wrap_token="1/OAuth2-Access_Token-nopqrstuvwxyz1234567890";
  const std::string expires_in="3600";

  const std::string data("{\n \"email\": \"someone@somewhere.net\",\n"
                         " \"verified_email\": true\n}\n");
  MockGaiaOAuthConsumer consumer;
  EXPECT_CALL(consumer,
              OnUserInfoSuccess(email_address)).Times(1);

  TestingProfile profile;
  MockGaiaOAuthFetcher auth(&consumer,
                            profile .GetRequestContext(),
                            &profile,
                            std::string());

  net::ResponseCookies cookies;
  net::URLRequestStatus status(net::URLRequestStatus::SUCCESS, 0);
  auth.OnURLFetchComplete(NULL,
                          GURL(kUserInfoUrl),
                          status,
                          RC_REQUEST_OK,
                          cookies,
                          data);
}

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/gaia/gaia_oauth_fetcher.h"

#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "chrome/browser/net/gaia/gaia_oauth_consumer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/net/gaia/gaia_auth_fetcher.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/net/gaia/gaia_urls.h"
#include "chrome/common/net/gaia/google_service_auth_error.h"
#include "chrome/common/net/gaia/oauth_request_signer.h"
#include "chrome/common/net/http_return.h"
#include "content/common/notification_details.h"
#include "content/common/notification_source.h"
#include "content/common/url_fetcher.h"
#include "grit/chromium_strings.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "ui/base/l10n/l10n_util.h"

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

static const char kOAuthTokenCookie[] = "oauth_token";

GaiaOAuthFetcher::GaiaOAuthFetcher(GaiaOAuthConsumer* consumer,
                                   net::URLRequestContextGetter* getter,
                                   Profile* profile,
                                   const std::string& service_scope)
    : consumer_(consumer),
      getter_(getter),
      profile_(profile),
      service_scope_(service_scope),
      popup_(NULL),
      fetch_pending_(false),
      auto_fetch_limit_(ALL_OAUTH_STEPS) {}

GaiaOAuthFetcher::~GaiaOAuthFetcher() {}

bool GaiaOAuthFetcher::HasPendingFetch() {
  return fetch_pending_;
}

void GaiaOAuthFetcher::CancelRequest() {
  fetcher_.reset();
  fetch_pending_ = false;
}

// static
URLFetcher* GaiaOAuthFetcher::CreateGaiaFetcher(
    net::URLRequestContextGetter* getter,
    const GURL& gaia_gurl,
    const std::string& body,
    const std::string& headers,
    bool send_cookies,
    URLFetcher::Delegate* delegate) {
  bool empty_body = body.empty();
  URLFetcher* result =
      URLFetcher::Create(0,
                         gaia_gurl,
                         empty_body ? URLFetcher::GET : URLFetcher::POST,
                         delegate);
  result->set_request_context(getter);

  // The Gaia/OAuth token exchange requests do not require any cookie-based
  // identification as part of requests.  We suppress sending any cookies to
  // maintain a separation between the user's browsing and Chrome's internal
  // services.  Where such mixing is desired (prelogin, autologin
  // or chromeos login), it will be done explicitly.
  if (!send_cookies)
    result->set_load_flags(net::LOAD_DO_NOT_SEND_COOKIES);

  if (!empty_body)
    result->set_upload_data("application/x-www-form-urlencoded", body);
  if (!headers.empty())
    result->set_extra_request_headers(headers);

  return result;
}

// static
GURL GaiaOAuthFetcher::MakeGetOAuthTokenUrl(
    const char* oauth1_login_scope,
    const std::string& product_name) {
  return GURL(std::string(kGetOAuthTokenUrl) +
      "?scope=" + oauth1_login_scope +
      "&xoauth_display_name=" +
      OAuthRequestSigner::Encode(product_name));
}

// static
std::string GaiaOAuthFetcher::MakeOAuthLoginBody(
    const char* source,
    const char* service,
    const std::string& oauth1_access_token,
    const std::string& oauth1_access_token_secret) {
  OAuthRequestSigner::Parameters parameters;
  parameters["service"] = service;
  parameters["source"] = source;
  std::string signed_request;
  bool is_signed = OAuthRequestSigner::Sign(
      GURL(kOAuth1LoginScope),
      parameters,
      OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      OAuthRequestSigner::POST_METHOD,
      "anonymous",  // oauth_consumer_key
      "anonymous",  // consumer secret
      oauth1_access_token,  // oauth_token
      oauth1_access_token_secret,  // token secret
      &signed_request);
  DCHECK(is_signed);
  return signed_request;
}

// static
std::string GaiaOAuthFetcher::MakeOAuthGetAccessTokenBody(
    const std::string& oauth1_request_token) {
  OAuthRequestSigner::Parameters empty_parameters;
  std::string signed_request;
  bool is_signed = OAuthRequestSigner::Sign(
      GURL(kOAuthGetAccessTokenUrl),
      empty_parameters,
      OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      OAuthRequestSigner::POST_METHOD,
      "anonymous",  // oauth_consumer_key
      "anonymous",  // consumer secret
      oauth1_request_token,  // oauth_token
      "",  // token secret
      &signed_request);
  DCHECK(is_signed);
  return signed_request;
}

// static
std::string GaiaOAuthFetcher::MakeOAuthWrapBridgeBody(
    const std::string& oauth1_access_token,
    const std::string& oauth1_access_token_secret,
    const std::string& wrap_token_duration,
    const std::string& oauth2_scope) {
  OAuthRequestSigner::Parameters parameters;
  parameters["wrap_token_duration"] = wrap_token_duration;
  parameters["wrap_scope"] = oauth2_scope;
  std::string signed_request;
  bool is_signed = OAuthRequestSigner::Sign(
      GURL(kOAuthWrapBridgeUrl),
      parameters,
      OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      OAuthRequestSigner::POST_METHOD,
      "anonymous",  // oauth_consumer_key
      "anonymous",  // consumer secret
      oauth1_access_token,  // oauth_token
      oauth1_access_token_secret,  // token secret
      &signed_request);
  DCHECK(is_signed);
  return signed_request;
}

// Helper method that extracts tokens from a successful reply.
// static
void GaiaOAuthFetcher::ParseGetOAuthTokenResponse(
    const std::string& data,
    std::string* token) {
  using std::vector;
  using std::pair;
  using std::string;

  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(data, '=', '&', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
       i != tokens.end(); ++i) {
    if (i->first == "oauth_token") {
      std::string decoded;
      if (OAuthRequestSigner::Decode(i->second, &decoded))
        token->assign(decoded);
    }
  }
}
// Helper method that extracts tokens from a successful reply.
// static
void GaiaOAuthFetcher::ParseOAuthLoginResponse(
    const std::string& data,
    std::string* sid,
    std::string* lsid,
    std::string* auth) {
  using std::vector;
  using std::pair;
  using std::string;
  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(data, '=', '\n', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
      i != tokens.end(); ++i) {
    if (i->first == "SID") {
      *sid = i->second;
    } else if (i->first == "LSID") {
      *lsid = i->second;
    } else if (i->first == "Auth") {
      *auth = i->second;
    }
  }
}

// Helper method that extracts tokens from a successful reply.
// static
void GaiaOAuthFetcher::ParseOAuthGetAccessTokenResponse(
    const std::string& data,
    std::string* token,
    std::string* secret) {
  using std::vector;
  using std::pair;
  using std::string;

  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(data, '=', '&', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
       i != tokens.end(); ++i) {
    if (i->first == "oauth_token") {
      std::string decoded;
      if (OAuthRequestSigner::Decode(i->second, &decoded))
        token->assign(decoded);
    } else if (i->first == "oauth_token_secret") {
      std::string decoded;
      if (OAuthRequestSigner::Decode(i->second, &decoded))
        secret->assign(decoded);
    }
  }
}

// Helper method that extracts tokens from a successful reply.
// static
void GaiaOAuthFetcher::ParseOAuthWrapBridgeResponse(const std::string& data,
                                                    std::string* token,
                                                    std::string* expires_in) {
  using std::vector;
  using std::pair;
  using std::string;

  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(data, '=', '&', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
       i != tokens.end(); ++i) {
    if (i->first == "wrap_access_token") {
      std::string decoded;
      if (OAuthRequestSigner::Decode(i->second, &decoded))
        token->assign(decoded);
    } else if (i->first == "wrap_access_token_expires_in") {
      std::string decoded;
      if (OAuthRequestSigner::Decode(i->second, &decoded))
        expires_in->assign(decoded);
    }
  }
}

// Helper method that extracts tokens from a successful reply.
// static
void GaiaOAuthFetcher::ParseUserInfoResponse(const std::string& data,
                                             std::string* email_result) {
  base::JSONReader reader;
  scoped_ptr<base::Value> value(reader.Read(data, false));
  if (value->GetType() == base::Value::TYPE_DICTIONARY) {
    Value* email_value;
    DictionaryValue* dict = static_cast<DictionaryValue*>(value.get());
    if (dict->Get("email", &email_value)) {
      if (email_value->GetType() == base::Value::TYPE_STRING) {
        StringValue* email = static_cast<StringValue*>(email_value);
        email->GetAsString(email_result);
      }
    }
  }
}

namespace {
// Based on Browser::OpenURLFromTab
void OpenGetOAuthTokenURL(Browser* browser,
                          const GURL& url,
                          const GURL& referrer,
                          WindowOpenDisposition disposition,
                          PageTransition::Type transition) {
  browser::NavigateParams params(
      browser,
      url,
      PageTransition::AUTO_BOOKMARK);
  params.source_contents =
      browser->tabstrip_model()->GetTabContentsAt(
          browser->tabstrip_model()->GetWrapperIndex(NULL));
  params.referrer = GURL("chrome://settings/personal");
  params.disposition = disposition;
  params.tabstrip_add_types = TabStripModel::ADD_NONE;
  params.window_action = browser::NavigateParams::SHOW_WINDOW;
  params.window_bounds = gfx::Rect(380, 520);
  params.user_gesture = true;
  browser::Navigate(&params);
}
}

void GaiaOAuthFetcher::StartGetOAuthToken() {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";
  DCHECK(!popup_);

  fetch_pending_ = true;
  registrar_.Add(this,
                 chrome::NOTIFICATION_COOKIE_CHANGED,
                 Source<Profile>(profile_));

  Browser* browser = BrowserList::GetLastActiveWithProfile(profile_);
  DCHECK(browser);

  OpenGetOAuthTokenURL(browser,
      MakeGetOAuthTokenUrl(kOAuth1LoginScope,
                           l10n_util::GetStringUTF8(IDS_PRODUCT_NAME)),
      GURL("chrome://settings/personal"),
      NEW_POPUP,
      PageTransition::AUTO_BOOKMARK);
  popup_ = BrowserList::GetLastActiveWithProfile(profile_);
  DCHECK(popup_ && popup_ != browser);
  registrar_.Add(this,
                 chrome::NOTIFICATION_BROWSER_CLOSING,
                 Source<Browser>(popup_));
}

void GaiaOAuthFetcher::StartOAuthLogin(
    const char* source,
    const char* service,
    const std::string& oauth1_access_token,
    const std::string& oauth1_access_token_secret) {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";

  // Must outlive fetcher_.
  request_body_ = MakeOAuthLoginBody(source, service, oauth1_access_token,
                                     oauth1_access_token_secret);
  request_headers_ = "";
  fetcher_.reset(CreateGaiaFetcher(getter_,
                                   GURL(kOAuth1LoginScope),
                                   request_body_,
                                   request_headers_,
                                   false,
                                   this));
  fetch_pending_ = true;
  fetcher_->Start();
}

void GaiaOAuthFetcher::StartGetOAuthTokenRequest() {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";

  // Must outlive fetcher_.
  request_body_ = "";
  request_headers_ = "";
  fetcher_.reset(CreateGaiaFetcher(getter_,
      MakeGetOAuthTokenUrl(kOAuth1LoginScope,
                           l10n_util::GetStringUTF8(IDS_PRODUCT_NAME)),
      std::string(),
      std::string(),
      true,           // send_cookies
      this));
  fetch_pending_ = true;
  fetcher_->Start();
}

void GaiaOAuthFetcher::StartOAuthGetAccessToken(
    const std::string& oauth1_request_token) {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";

  // Must outlive fetcher_.
  request_body_ = MakeOAuthGetAccessTokenBody(oauth1_request_token);
  request_headers_ = "";
  fetcher_.reset(CreateGaiaFetcher(getter_,
                                   GURL(kOAuthGetAccessTokenUrl),
                                   request_body_,
                                   request_headers_,
                                   false,
                                   this));
  fetch_pending_ = true;
  fetcher_->Start();
}

void GaiaOAuthFetcher::StartOAuthWrapBridge(
    const std::string& oauth1_access_token,
    const std::string& oauth1_access_token_secret,
    const std::string& wrap_token_duration,
    const std::string& oauth2_scope) {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";

  std::string combined_oauth2_scope = oauth2_scope + " " +
      kOAuthWrapBridgeUserInfoScope;

  // Must outlive fetcher_.
  request_body_ = MakeOAuthWrapBridgeBody(
      oauth1_access_token,
      oauth1_access_token_secret,
      wrap_token_duration,
      combined_oauth2_scope);

  request_headers_ = "";
  fetcher_.reset(CreateGaiaFetcher(getter_,
                                   GURL(kOAuthWrapBridgeUrl),
                                   request_body_,
                                   request_headers_,
                                   false,
                                   this));
  fetch_pending_ = true;
  fetcher_->Start();
}

void GaiaOAuthFetcher::StartUserInfo(const std::string& oauth2_access_token) {
  DCHECK(!fetch_pending_) << "Tried to fetch two things at once!";

  // Must outlive fetcher_.
  request_body_ = "";
  request_headers_ = "Authorization: OAuth " + oauth2_access_token;
  fetcher_.reset(CreateGaiaFetcher(getter_,
                                   GURL(kUserInfoUrl),
                                   request_body_,
                                   request_headers_,
                                   false,
                                   this));
  fetch_pending_ = true;
  fetcher_->Start();
}

// static
GoogleServiceAuthError GaiaOAuthFetcher::GenerateAuthError(
    const std::string& data,
    const net::URLRequestStatus& status) {
  if (!status.is_success()) {
    if (status.status() == net::URLRequestStatus::CANCELED) {
      return GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED);
    } else {
      LOG(WARNING) << "Could not reach Google Accounts servers: errno "
                   << status.os_error();
      return GoogleServiceAuthError::FromConnectionError(status.os_error());
    }
  } else {
    LOG(WARNING) << "Unrecognized response from Google Accounts servers.";
    return GoogleServiceAuthError(
        GoogleServiceAuthError::SERVICE_UNAVAILABLE);
  }

  NOTREACHED();
  return GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE);
}

void GaiaOAuthFetcher::Observe(int type,
                               const NotificationSource& source,
                               const NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_COOKIE_CHANGED: {
      OnCookieChanged(Source<Profile>(source).ptr(),
                      Details<ChromeCookieDetails>(details).ptr());
      break;
    }
    case chrome::NOTIFICATION_BROWSER_CLOSING: {
      OnBrowserClosing(Source<Browser>(source).ptr(),
                       *(Details<bool>(details)).ptr());
      break;
    }
    default: {
      NOTREACHED();
    }
  }
}

namespace {
  const char* CauseName(net::CookieMonster::Delegate::ChangeCause cause) {
    switch (cause) {
      case net::CookieMonster::Delegate::CHANGE_COOKIE_EXPLICIT:
        return "CHANGE_COOKIE_EXPLICIT";
      case net::CookieMonster::Delegate::CHANGE_COOKIE_OVERWRITE:
        return "CHANGE_COOKIE_OVERWRITE";
      case net::CookieMonster::Delegate::CHANGE_COOKIE_EXPIRED:
        return "CHANGE_COOKIE_EXPIRED";
      case net::CookieMonster::Delegate::CHANGE_COOKIE_EVICTED:
        return "CHANGE_COOKIE_EVICTED";
      case net::CookieMonster::Delegate::CHANGE_COOKIE_EXPIRED_OVERWRITE:
        return "CHANGE_COOKIE_EXPIRED_OVERWRITE";
      default:
        return "<unknown>";
    }
  }
}

void GaiaOAuthFetcher::OnBrowserClosing(Browser* browser,
                                        bool detail) {
  if (browser == popup_) {
    consumer_->OnGetOAuthTokenFailure();
  }
  popup_ = NULL;
}

void GaiaOAuthFetcher::OnCookieChanged(Profile* profile,
                                       ChromeCookieDetails* cookie_details) {
  const net::CookieMonster::CanonicalCookie* canonical_cookie =
      cookie_details->cookie;
  if (canonical_cookie->Name() == kOAuthTokenCookie) {
    fetch_pending_ = false;
    OnGetOAuthTokenFetched(canonical_cookie->Value());
  }
}

void GaiaOAuthFetcher::OnGetOAuthTokenFetched(const std::string& token) {
  // DCHECK (popup_);
  if (popup_) {
    Browser* popped_up = popup_;
    popup_ = NULL;
    popped_up->CloseWindow();
  }
  consumer_->OnGetOAuthTokenSuccess(token);
  if (ShouldAutoFetch(OAUTH1_ALL_ACCESS_TOKEN))
    StartOAuthGetAccessToken(token);
}

void GaiaOAuthFetcher::OnGetOAuthTokenUrlFetched(
    const net::ResponseCookies& cookies,
    const net::URLRequestStatus& status,
    int response_code) {
  if (status.is_success() && response_code == RC_REQUEST_OK) {
    for (net::ResponseCookies::const_iterator iter = cookies.begin();
        iter != cookies.end(); ++iter) {
      net::CookieMonster::ParsedCookie cookie(*iter);
      if (cookie.Name() == kOAuthTokenCookie) {
        std::string token = cookie.Value();
        consumer_->OnGetOAuthTokenSuccess(token);
        if (ShouldAutoFetch(OAUTH1_ALL_ACCESS_TOKEN))
          StartOAuthGetAccessToken(token);
        return;
      }
    }
  } else {
    consumer_->OnGetOAuthTokenFailure();
  }
}

void GaiaOAuthFetcher::OnOAuthLoginFetched(
    const std::string& data,
    const net::URLRequestStatus& status,
    int response_code) {
  if (status.is_success() && response_code == RC_REQUEST_OK) {
    std::string sid;
    std::string lsid;
    std::string auth;
    ParseOAuthLoginResponse(data, &sid, &lsid, &auth);
    consumer_->OnOAuthLoginSuccess(sid, lsid, auth);
  } else {
    // OAuthLogin returns error messages that are identical to ClientLogin,
    // so we use GaiaAuthFetcher::GenerateAuthError to parse the response
    // instead.
    consumer_->OnOAuthLoginFailure(
        GaiaAuthFetcher::GenerateOAuthLoginError(data, status));
  }
}

void GaiaOAuthFetcher::OnOAuthGetAccessTokenFetched(
    const std::string& data,
    const net::URLRequestStatus& status,
    int response_code) {
  if (status.is_success() && response_code == RC_REQUEST_OK) {
    std::string secret;
    std::string token;
    ParseOAuthGetAccessTokenResponse(data, &token, &secret);
    consumer_->OnOAuthGetAccessTokenSuccess(token, secret);
    if (ShouldAutoFetch(OAUTH2_SERVICE_ACCESS_TOKEN))
      StartOAuthWrapBridge(token, secret, "3600", service_scope_);
  } else {
    consumer_->OnOAuthGetAccessTokenFailure(GenerateAuthError(data, status));
  }
}

void GaiaOAuthFetcher::OnOAuthWrapBridgeFetched(
    const std::string& data,
    const net::URLRequestStatus& status,
    int response_code) {
  if (status.is_success() && response_code == RC_REQUEST_OK) {
    std::string token;
    std::string expires_in;
    ParseOAuthWrapBridgeResponse(data, &token, &expires_in);
    consumer_->OnOAuthWrapBridgeSuccess(token, expires_in);
    if (ShouldAutoFetch(USER_INFO))
      StartUserInfo(token);
  } else {
    consumer_->OnOAuthWrapBridgeFailure(GenerateAuthError(data, status));
  }
}

void GaiaOAuthFetcher::OnUserInfoFetched(
    const std::string& data,
    const net::URLRequestStatus& status,
    int response_code) {
  if (status.is_success() && response_code == RC_REQUEST_OK) {
    std::string email;
    ParseUserInfoResponse(data, &email);
    consumer_->OnUserInfoSuccess(email);
  } else {
    consumer_->OnUserInfoFailure(GenerateAuthError(data, status));
  }
}

void GaiaOAuthFetcher::OnURLFetchComplete(const URLFetcher* source,
                                          const GURL& url,
                                          const net::URLRequestStatus& status,
                                          int response_code,
                                          const net::ResponseCookies& cookies,
                                          const std::string& data) {
  fetch_pending_ = false;
  if (StartsWithASCII(url.spec(), kGetOAuthTokenUrl, true)) {
    OnGetOAuthTokenUrlFetched(cookies, status, response_code);
  } else if (url.spec() == kOAuth1LoginScope) {
    OnOAuthLoginFetched(data, status, response_code);
  } else if (url.spec() == kOAuthGetAccessTokenUrl) {
    OnOAuthGetAccessTokenFetched(data, status, response_code);
  } else if (url.spec() == kOAuthWrapBridgeUrl) {
    OnOAuthWrapBridgeFetched(data, status, response_code);
  } else if (url.spec() == kUserInfoUrl) {
    OnUserInfoFetched(data, status, response_code);
  } else {
    NOTREACHED();
  }
}

bool GaiaOAuthFetcher::ShouldAutoFetch(AutoFetchLimit fetch_step) {
  return fetch_step <= auto_fetch_limit_;
}

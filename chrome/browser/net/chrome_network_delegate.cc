// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_network_delegate.h"

#include "base/logging.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/extensions/extension_event_router_forwarder.h"
#include "chrome/browser/extensions/extension_info_map.h"
#include "chrome/browser/extensions/extension_proxy_api.h"
#include "chrome/browser/extensions/extension_webrequest_api.h"
#include "chrome/browser/prefs/pref_member.h"
#include "chrome/browser/task_manager/task_manager.h"
#include "chrome/common/pref_names.h"
#include "content/browser/browser_thread.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace {

// If the |request| failed due to problems with a proxy, forward the error to
// the proxy extension API.
void ForwardProxyErrors(net::URLRequest* request,
                        ExtensionEventRouterForwarder* event_router,
                        void* profile) {
  if (request->status().status() == net::URLRequestStatus::FAILED) {
    switch (request->status().os_error()) {
      case net::ERR_PROXY_AUTH_UNSUPPORTED:
      case net::ERR_PROXY_CONNECTION_FAILED:
      case net::ERR_TUNNEL_CONNECTION_FAILED:
        ExtensionProxyEventRouter::GetInstance()->OnProxyError(
            event_router, profile, request->status().os_error());
    }
  }
}

}  // namespace

ChromeNetworkDelegate::ChromeNetworkDelegate(
    ExtensionEventRouterForwarder* event_router,
    ExtensionInfoMap* extension_info_map,
    void* profile,
    BooleanPrefMember* enable_referrers)
    : event_router_(event_router),
      profile_(profile),
      extension_info_map_(extension_info_map),
      enable_referrers_(enable_referrers) {
  DCHECK(event_router);
  DCHECK(enable_referrers);
}

ChromeNetworkDelegate::~ChromeNetworkDelegate() {}

// static
void ChromeNetworkDelegate::InitializeReferrersEnabled(
    BooleanPrefMember* enable_referrers,
    PrefService* pref_service) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  enable_referrers->Init(prefs::kEnableReferrers, pref_service, NULL);
  enable_referrers->MoveToThread(BrowserThread::IO);
}

int ChromeNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    net::CompletionCallback* callback,
    GURL* new_url) {
  if (!enable_referrers_->GetValue())
    request->set_referrer(std::string());
  return ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRequest(
      profile_, extension_info_map_.get(), request, callback, new_url);
}

int ChromeNetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    net::CompletionCallback* callback,
    net::HttpRequestHeaders* headers) {
  return ExtensionWebRequestEventRouter::GetInstance()->OnBeforeSendHeaders(
      profile_, extension_info_map_.get(), request, callback, headers);
}

void ChromeNetworkDelegate::OnSendHeaders(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  ExtensionWebRequestEventRouter::GetInstance()->OnSendHeaders(
      profile_, extension_info_map_.get(), request, headers);
}

void ChromeNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                             const GURL& new_location) {
  ExtensionWebRequestEventRouter::GetInstance()->OnBeforeRedirect(
      profile_, extension_info_map_.get(), request, new_location);
}


void ChromeNetworkDelegate::OnResponseStarted(net::URLRequest* request) {
  ExtensionWebRequestEventRouter::GetInstance()->OnResponseStarted(
      profile_, extension_info_map_.get(), request);
  ForwardProxyErrors(request, event_router_.get(), profile_);
}

void ChromeNetworkDelegate::OnRawBytesRead(const net::URLRequest& request,
                                           int bytes_read) {
  TaskManager::GetInstance()->model()->NotifyBytesRead(request, bytes_read);
}

void ChromeNetworkDelegate::OnCompleted(net::URLRequest* request) {
  if (request->status().status() == net::URLRequestStatus::SUCCESS ||
      request->status().status() == net::URLRequestStatus::HANDLED_EXTERNALLY) {
    bool is_redirect = request->response_headers() &&
        net::HttpResponseHeaders::IsRedirectResponseCode(
            request->response_headers()->response_code());
    if (!is_redirect) {
      ExtensionWebRequestEventRouter::GetInstance()->OnCompleted(
          profile_, extension_info_map_.get(), request);
    }
  } else if (request->status().status() == net::URLRequestStatus::FAILED ||
             request->status().status() == net::URLRequestStatus::CANCELED) {
    ExtensionWebRequestEventRouter::GetInstance()->OnErrorOccurred(
            profile_, extension_info_map_.get(), request);
  } else {
    NOTREACHED();
  }
  ForwardProxyErrors(request, event_router_.get(), profile_);
}

void ChromeNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
  ExtensionWebRequestEventRouter::GetInstance()->OnURLRequestDestroyed(
      profile_, request);
}

void ChromeNetworkDelegate::OnPACScriptError(int line_number,
                                             const string16& error) {
  ExtensionProxyEventRouter::GetInstance()->OnPACScriptError(
      event_router_.get(), profile_, line_number, error);
}

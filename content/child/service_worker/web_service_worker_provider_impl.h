// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_PROVIDER_IMPL_H_
#define CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_PROVIDER_IMPL_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "third_party/WebKit/public/platform/WebServiceWorkerProvider.h"

namespace blink {
class WebURL;
class WebServiceWorkerProviderClient;
}

namespace content {

class ServiceWorkerDispatcher;
class ThreadSafeSender;

class WebServiceWorkerProviderImpl
    : NON_EXPORTED_BASE(public blink::WebServiceWorkerProvider) {
 public:
  WebServiceWorkerProviderImpl(ThreadSafeSender* thread_safe_sender,
                               int provider_id);
  virtual ~WebServiceWorkerProviderImpl();

  virtual void setClient(blink::WebServiceWorkerProviderClient* client);

  virtual void registerServiceWorker(const blink::WebURL& pattern,
                                     const blink::WebURL& script_url,
                                     WebServiceWorkerCallbacks*);

  virtual void unregisterServiceWorker(const blink::WebURL& pattern,
                                       WebServiceWorkerCallbacks*);

 private:
  ServiceWorkerDispatcher* GetDispatcher();

  scoped_refptr<ThreadSafeSender> thread_safe_sender_;
  const int provider_id_;

  DISALLOW_COPY_AND_ASSIGN(WebServiceWorkerProviderImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_SERVICE_WORKER_WEB_SERVICE_WORKER_PROVIDER_IMPL_H_

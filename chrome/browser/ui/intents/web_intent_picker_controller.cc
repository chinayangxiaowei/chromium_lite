// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/intents/web_intent_picker_controller.h"

#include <vector>

#include "chrome/browser/favicon/favicon_service.h"
#include "chrome/browser/intents/web_intent_data.h"
#include "chrome/browser/intents/web_intents_registry.h"
#include "chrome/browser/intents/web_intents_registry_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/intents/web_intent_picker.h"
#include "chrome/browser/ui/intents/web_intent_picker_factory.h"
#include "chrome/browser/webdata/web_data_service.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/notification_source.h"
#include "ui/gfx/codec/png_codec.h"

namespace {

// Gets the favicon service for the profile in |tab_contents|.
FaviconService* GetFaviconService(TabContents* tab_contents) {
  Profile* profile = Profile::FromBrowserContext(
      tab_contents->browser_context());
  return profile->GetFaviconService(Profile::EXPLICIT_ACCESS);
}

// Gets the web intents registry for the profile in |tab_contents|.
WebIntentsRegistry* GetWebIntentsRegistry(TabContents* tab_contents) {
  Profile* profile = Profile::FromBrowserContext(
      tab_contents->browser_context());
  return WebIntentsRegistryFactory::GetForProfile(profile);
}

}  // namespace

// A class that asynchronously fetches web intent data from the web intents
// registry.
class WebIntentPickerController::WebIntentDataFetcher
    : public WebIntentsRegistry::Consumer {
 public:
  WebIntentDataFetcher(WebIntentPickerController* controller,
                       WebIntentsRegistry* web_intents_registry);
  ~WebIntentDataFetcher();

  // Asynchronously fetches WebIntentData for the given |action| |type| pair.
  void Fetch(const string16& action, const string16& type);

  // Cancels the WebDataService request.
  void Cancel();

 private:
  // WebIntentsRegistry::Consumer implementation.
  virtual void OnIntentsQueryDone(
      WebIntentsRegistry::QueryID,
      const std::vector<WebIntentData>& intents) OVERRIDE;

  // A weak pointer to the picker controller.
  WebIntentPickerController* controller_;

  // A weak pointer to the web intents registry.
  WebIntentsRegistry* web_intents_registry_;

  // The ID of the current web intents request. The value will be -1 if
  // there is no request in flight.
  WebIntentsRegistry::QueryID query_id_;
};

// A class that asynchronously fetches favicons for a vector of URLs.
class WebIntentPickerController::FaviconFetcher {
 public:
  FaviconFetcher(WebIntentPickerController* controller,
                 FaviconService *favicon_service);
  ~FaviconFetcher();

  // Asynchronously fetches favicons for the each URL in |urls|.
  void Fetch(const std::vector<GURL>& urls);

  // Cancels all favicon requests.
  void Cancel();

 private:
  // Callback called when a favicon is received.
  void OnFaviconDataAvailable(FaviconService::Handle handle,
                              history::FaviconData favicon_data);

  // A weak pointer to the picker controller.
  WebIntentPickerController* controller_;

  // A weak pointer to the favicon service.
  FaviconService* favicon_service_;

  // The Consumer to handle asynchronous favicon requests.
  CancelableRequestConsumerTSimple<size_t> load_consumer_;

  DISALLOW_COPY_AND_ASSIGN(FaviconFetcher);
};

WebIntentPickerController::WebIntentPickerController(
    TabContents* tab_contents,
    WebIntentPickerFactory* factory)
        : tab_contents_(tab_contents),
          picker_factory_(factory),
          web_intent_data_fetcher_(
              new WebIntentDataFetcher(this,
                                       GetWebIntentsRegistry(tab_contents))),
          favicon_fetcher_(
              new FaviconFetcher(this, GetFaviconService(tab_contents))),
          picker_(NULL),
          pending_async_count_(0) {
  NavigationController* controller = &tab_contents->controller();
  registrar_.Add(this, content::NOTIFICATION_LOAD_START,
                 Source<NavigationController>(controller));
  registrar_.Add(this, content::NOTIFICATION_TAB_CLOSING,
                 Source<NavigationController>(controller));
}

WebIntentPickerController::~WebIntentPickerController() {
}

void WebIntentPickerController::ShowDialog(const string16& action,
                                           const string16& type) {
  if (picker_ != NULL)
    return;

  picker_ = picker_factory_->Create(tab_contents_, this);

  // TODO(binji) Remove this check when there are implementations of the picker
  // for windows and mac.
  if (picker_ == NULL)
    return;

  web_intent_data_fetcher_->Fetch(action, type);
}

void WebIntentPickerController::Observe(int type,
                                        const NotificationSource& source,
                                        const NotificationDetails& details) {
  DCHECK(type == content::NOTIFICATION_LOAD_START ||
         type == content::NOTIFICATION_TAB_CLOSING);
  ClosePicker();
}

void WebIntentPickerController::OnServiceChosen(size_t index) {
  DCHECK(index < urls_.size());

  // TODO(binji) Send the chosen service back to the renderer.
  LOG(INFO) << "Chose index: " << index << " url: " << urls_[index];
  ClosePicker();
}

void WebIntentPickerController::OnCancelled() {
  // TODO(binji) Tell the renderer that the intent was cancelled.
  ClosePicker();
}

void WebIntentPickerController::OnWebIntentDataAvailable(
    const std::vector<WebIntentData>& intent_data) {
  urls_.clear();
  for (size_t i = 0; i < intent_data.size(); ++i) {
    urls_.push_back(intent_data[i].service_url);
  }

  // Tell the picker to initialize N urls to the default favicon
  picker_->SetServiceURLs(urls_);
  favicon_fetcher_->Fetch(urls_);
  pending_async_count_--;
}

void WebIntentPickerController::OnFaviconDataAvailable(
    size_t index,
    const SkBitmap& icon_bitmap) {
  picker_->SetServiceIcon(index, icon_bitmap);
  pending_async_count_--;
}

void WebIntentPickerController::OnFaviconDataUnavailable(size_t index) {
  picker_->SetDefaultServiceIcon(index);
  pending_async_count_--;
}

void WebIntentPickerController::ClosePicker() {
  if (picker_) {
    picker_factory_->ClosePicker(picker_);
    picker_ = NULL;
  }
}

WebIntentPickerController::WebIntentDataFetcher::WebIntentDataFetcher(
    WebIntentPickerController* controller,
    WebIntentsRegistry* web_intents_registry)
        : controller_(controller),
          web_intents_registry_(web_intents_registry),
          query_id_(-1) {
}

WebIntentPickerController::WebIntentDataFetcher::~WebIntentDataFetcher() {
}

void WebIntentPickerController::WebIntentDataFetcher::Fetch(
    const string16& action,
    const string16& type) {
  DCHECK(query_id_ == -1) << "Fetch already in process.";
  controller_->pending_async_count_++;
  query_id_ = web_intents_registry_->GetIntentProviders(action, this);
}

void WebIntentPickerController::WebIntentDataFetcher::OnIntentsQueryDone(
    WebIntentsRegistry::QueryID,
    const std::vector<WebIntentData>& intents) {
  controller_->OnWebIntentDataAvailable(intents);
  query_id_ = -1;
}

WebIntentPickerController::FaviconFetcher::FaviconFetcher(
    WebIntentPickerController* controller,
    FaviconService* favicon_service)
        : controller_(controller),
          favicon_service_(favicon_service) {
}

WebIntentPickerController::FaviconFetcher::~FaviconFetcher() {
}

void WebIntentPickerController::FaviconFetcher::Fetch(
    const std::vector<GURL>& urls) {
  if (!favicon_service_)
    return;

  for (size_t index = 0; index < urls.size(); ++index) {
    controller_->pending_async_count_++;
    FaviconService::Handle handle = favicon_service_->GetFaviconForURL(
        urls[index],
        history::FAVICON,
        &load_consumer_,
        NewCallback(this, &WebIntentPickerController::FaviconFetcher::
            OnFaviconDataAvailable));
    load_consumer_.SetClientData(favicon_service_, handle, index);
  }
}

void WebIntentPickerController::FaviconFetcher::Cancel() {
  load_consumer_.CancelAllRequests();
}

void WebIntentPickerController::FaviconFetcher::OnFaviconDataAvailable(
    FaviconService::Handle handle,
    history::FaviconData favicon_data) {
  size_t index = load_consumer_.GetClientDataForCurrentRequest();
  if (favicon_data.is_valid()) {
    SkBitmap icon_bitmap;

    if (gfx::PNGCodec::Decode(favicon_data.image_data->front(),
                              favicon_data.image_data->size(),
                              &icon_bitmap)) {
      controller_->OnFaviconDataAvailable(index, icon_bitmap);
      return;
    }
  }

  controller_->OnFaviconDataUnavailable(index);
}

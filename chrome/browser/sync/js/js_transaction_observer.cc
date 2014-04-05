// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/js/js_transaction_observer.h"

#include <sstream>
#include <string>

#include "base/logging.h"
#include "base/tracked.h"
#include "base/values.h"
#include "chrome/browser/sync/js/js_event_details.h"
#include "chrome/browser/sync/js/js_event_handler.h"

namespace browser_sync {

JsTransactionObserver::JsTransactionObserver() {}

JsTransactionObserver::~JsTransactionObserver() {
  DCHECK(non_thread_safe_.CalledOnValidThread());
}

void JsTransactionObserver::SetJsEventHandler(
    const WeakHandle<JsEventHandler>& event_handler) {
  event_handler_ = event_handler;
}

namespace {

std::string GetLocationString(const tracked_objects::Location& location) {
  std::ostringstream oss;
  oss << location.function_name() << "@"
      << location.file_name() << ":" << location.line_number();
  return oss.str();
}

}  // namespace

void JsTransactionObserver::OnTransactionStart(
    const tracked_objects::Location& location,
    const syncable::WriterTag& writer) {
  DCHECK(non_thread_safe_.CalledOnValidThread());
  if (!event_handler_.IsInitialized()) {
    return;
  }
  DictionaryValue details;
  details.SetString("location", GetLocationString(location));
  details.SetString("writer", syncable::WriterTagToString(writer));
  HandleJsEvent(FROM_HERE, "onTransactionStart", JsEventDetails(&details));
}

void JsTransactionObserver::OnTransactionMutate(
    const tracked_objects::Location& location,
    const syncable::WriterTag& writer,
    const syncable::EntryKernelMutationSet& mutations,
    const syncable::ModelTypeBitSet& models_with_changes) {
  DCHECK(non_thread_safe_.CalledOnValidThread());
  if (!event_handler_.IsInitialized()) {
    return;
  }
  DictionaryValue details;
  details.SetString("location", GetLocationString(location));
  details.SetString("writer", syncable::WriterTagToString(writer));
  details.Set("mutations", syncable::EntryKernelMutationSetToValue(mutations));
  details.Set("modelsWithChanges",
              syncable::ModelTypeBitSetToValue(models_with_changes));
  HandleJsEvent(FROM_HERE, "onTransactionMutate", JsEventDetails(&details));
}

void JsTransactionObserver::OnTransactionEnd(
    const tracked_objects::Location& location,
    const syncable::WriterTag& writer) {
  DCHECK(non_thread_safe_.CalledOnValidThread());
  if (!event_handler_.IsInitialized()) {
    return;
  }
  DictionaryValue details;
  details.SetString("location", GetLocationString(location));
  details.SetString("writer", syncable::WriterTagToString(writer));
  HandleJsEvent(FROM_HERE, "onTransactionEnd", JsEventDetails(&details));
}

void JsTransactionObserver::HandleJsEvent(
    const tracked_objects::Location& from_here,
    const std::string& name, const JsEventDetails& details) {
  if (!event_handler_.IsInitialized()) {
    NOTREACHED();
    return;
  }
  event_handler_.Call(from_here,
                      &JsEventHandler::HandleJsEvent, name, details);
}

}  // namespace browser_sync

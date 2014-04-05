// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements the Chrome Extensions WebNavigation API.

#include "chrome/browser/extensions/extension_webnavigation_api.h"

#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/string_number_conversions.h"
#include "base/time.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_event_router.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/extensions/extension_webnavigation_api_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/url_constants.h"
#include "content/browser/tab_contents/navigation_details.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/notification_service.h"
#include "net/base/net_errors.h"

namespace keys = extension_webnavigation_api_constants;

namespace {

typedef std::map<TabContents*, ExtensionWebNavigationTabObserver*>
    TabObserverMap;
static base::LazyInstance<TabObserverMap> g_tab_observer(
    base::LINKER_INITIALIZED);

// URL schemes for which we'll send events.
const char* kValidSchemes[] = {
  chrome::kHttpScheme,
  chrome::kHttpsScheme,
  chrome::kFileScheme,
  chrome::kFtpScheme,
};

// Returns the frame ID as it will be passed to the extension:
// 0 if the navigation happens in the main frame, or the frame ID
// modulo 32 bits otherwise.
// Keep this in sync with the GetFrameId() function in
// extension_webrequest_api.cc.
int GetFrameId(bool is_main_frame, int64 frame_id) {
  return is_main_frame ? 0 : static_cast<int>(frame_id);
}

// Returns |time| as milliseconds since the epoch.
double MilliSecondsFromTime(const base::Time& time) {
  return 1000 * time.ToDoubleT();
}

// Dispatches events to the extension message service.
void DispatchEvent(Profile* profile,
                   const char* event_name,
                   const std::string& json_args) {
  if (profile && profile->GetExtensionEventRouter()) {
    profile->GetExtensionEventRouter()->DispatchEventToRenderers(
        event_name, json_args, profile, GURL());
  }
}

// Constructs and dispatches an onBeforeNavigate event.
void DispatchOnBeforeNavigate(TabContents* tab_contents,
                              int64 frame_id,
                              bool is_main_frame,
                              const GURL& validated_url) {
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents));
  dict->SetString(keys::kUrlKey, validated_url.spec());
  dict->SetInteger(keys::kFrameIdKey, GetFrameId(is_main_frame, frame_id));
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(tab_contents->profile(), keys::kOnBeforeNavigate, json_args);
}

// Constructs and dispatches an onCommitted event.
void DispatchOnCommitted(TabContents* tab_contents,
                         int64 frame_id,
                         bool is_main_frame,
                         const GURL& url,
                         PageTransition::Type transition_type) {
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents));
  dict->SetString(keys::kUrlKey, url.spec());
  dict->SetInteger(keys::kFrameIdKey, GetFrameId(is_main_frame, frame_id));
  dict->SetString(keys::kTransitionTypeKey,
                  PageTransition::CoreTransitionString(transition_type));
  ListValue* qualifiers = new ListValue();
  if (transition_type & PageTransition::CLIENT_REDIRECT)
    qualifiers->Append(Value::CreateStringValue("client_redirect"));
  if (transition_type & PageTransition::SERVER_REDIRECT)
    qualifiers->Append(Value::CreateStringValue("server_redirect"));
  if (transition_type & PageTransition::FORWARD_BACK)
    qualifiers->Append(Value::CreateStringValue("forward_back"));
  dict->Set(keys::kTransitionQualifiersKey, qualifiers);
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(tab_contents->profile(), keys::kOnCommitted, json_args);
}

// Constructs and dispatches an onDOMContentLoaded event.
void DispatchOnDOMContentLoaded(TabContents* tab_contents,
                                const GURL& url,
                                bool is_main_frame,
                                int64 frame_id) {
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents));
  dict->SetString(keys::kUrlKey, url.spec());
  dict->SetInteger(keys::kFrameIdKey, GetFrameId(is_main_frame, frame_id));
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(tab_contents->profile(), keys::kOnDOMContentLoaded, json_args);
}

// Constructs and dispatches an onCompleted event.
void DispatchOnCompleted(TabContents* tab_contents,
                         const GURL& url,
                         bool is_main_frame,
                         int64 frame_id) {
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents));
  dict->SetString(keys::kUrlKey, url.spec());
  dict->SetInteger(keys::kFrameIdKey, GetFrameId(is_main_frame, frame_id));
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(tab_contents->profile(), keys::kOnCompleted, json_args);
}

// Constructs and dispatches an onBeforeRetarget event.
void DispatchOnBeforeRetarget(TabContents* tab_contents,
                              Profile* profile,
                              int64 source_frame_id,
                              bool source_frame_is_main_frame,
                              TabContents* target_tab_contents,
                              const GURL& target_url) {
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kSourceTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents));
  dict->SetInteger(keys::kSourceFrameIdKey,
      GetFrameId(source_frame_is_main_frame, source_frame_id));
  dict->SetString(keys::kUrlKey, target_url.possibly_invalid_spec());
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(target_tab_contents));
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(profile, keys::kOnBeforeRetarget, json_args);
}

}  // namespace


// FrameNavigationState -------------------------------------------------------

// static
bool FrameNavigationState::allow_extension_scheme_ = false;

FrameNavigationState::FrameNavigationState() {}

FrameNavigationState::~FrameNavigationState() {}

bool FrameNavigationState::CanSendEvents(int64 frame_id) const {
  FrameIdToStateMap::const_iterator frame_state =
      frame_state_map_.find(frame_id);
  if (frame_state == frame_state_map_.end() ||
      frame_state->second.error_occurred) {
    return false;
  }
  const std::string& scheme = frame_state->second.url.scheme();
  for (unsigned i = 0; i < arraysize(kValidSchemes); ++i) {
    if (scheme == kValidSchemes[i])
      return true;
  }
  if (allow_extension_scheme_ && scheme == chrome::kExtensionScheme)
    return true;
  return false;
}

void FrameNavigationState::TrackFrame(int64 frame_id,
                                      const GURL& url,
                                      bool is_main_frame,
                                      bool is_error_page) {
  if (is_main_frame)
    frame_state_map_.clear();
  FrameState& frame_state = frame_state_map_[frame_id];
  frame_state.error_occurred = is_error_page;
  frame_state.url = url;
  frame_state.is_main_frame = is_main_frame;
}

GURL FrameNavigationState::GetUrl(int64 frame_id) const {
  FrameIdToStateMap::const_iterator frame_state =
      frame_state_map_.find(frame_id);
  if (frame_state == frame_state_map_.end()) {
    NOTREACHED();
    return GURL();
  }
  return frame_state->second.url;
}

bool FrameNavigationState::IsMainFrame(int64 frame_id) const {
  FrameIdToStateMap::const_iterator frame_state =
      frame_state_map_.find(frame_id);
  if (frame_state == frame_state_map_.end()) {
    NOTREACHED();
    return false;
  }
  return frame_state->second.is_main_frame;
}

int64 FrameNavigationState::GetMainFrameID() const {
  typedef FrameIdToStateMap::const_iterator FrameIterator;
  for (FrameIterator frame = frame_state_map_.begin();
       frame != frame_state_map_.end(); ++frame) {
    if (frame->second.is_main_frame)
      return frame->first;
  }
  return -1;
}

void FrameNavigationState::ErrorOccurredInFrame(int64 frame_id) {
  DCHECK(frame_state_map_.find(frame_id) != frame_state_map_.end());
  frame_state_map_[frame_id].error_occurred = true;
}


// ExtensionWebNavigtionEventRouter -------------------------------------------

ExtensionWebNavigationEventRouter::PendingTabContents::PendingTabContents()
    : source_tab_contents(NULL),
      source_frame_id(0),
      source_frame_is_main_frame(false),
      target_tab_contents(NULL),
      target_url() {
}

ExtensionWebNavigationEventRouter::PendingTabContents::PendingTabContents(
    TabContents* source_tab_contents,
    int64 source_frame_id,
    bool source_frame_is_main_frame,
    TabContents* target_tab_contents,
    const GURL& target_url)
    : source_tab_contents(source_tab_contents),
      source_frame_id(source_frame_id),
      source_frame_is_main_frame(source_frame_is_main_frame),
      target_tab_contents(target_tab_contents),
      target_url(target_url) {
}

ExtensionWebNavigationEventRouter::PendingTabContents::~PendingTabContents() {}

ExtensionWebNavigationEventRouter::ExtensionWebNavigationEventRouter(
    Profile* profile) : profile_(profile) {}

ExtensionWebNavigationEventRouter::~ExtensionWebNavigationEventRouter() {}

void ExtensionWebNavigationEventRouter::Init() {
  if (registrar_.IsEmpty()) {
    registrar_.Add(this,
                   content::NOTIFICATION_RETARGETING,
                   Source<Profile>(profile_));
    registrar_.Add(this,
                   content::NOTIFICATION_TAB_ADDED,
                   NotificationService::AllSources());
    registrar_.Add(this,
                   content::NOTIFICATION_TAB_CONTENTS_DESTROYED,
                   NotificationService::AllSources());
  }
}

void ExtensionWebNavigationEventRouter::Observe(
    int type,
    const NotificationSource& source,
    const NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_RETARGETING:
      Retargeting(Details<const content::RetargetingDetails>(details).ptr());
      break;

    case content::NOTIFICATION_TAB_ADDED:
      TabAdded(Details<TabContents>(details).ptr());
      break;

    case content::NOTIFICATION_TAB_CONTENTS_DESTROYED:
      TabDestroyed(Source<TabContents>(source).ptr());
      break;

    default:
      NOTREACHED();
  }
}

void ExtensionWebNavigationEventRouter::Retargeting(
    const content::RetargetingDetails* details) {
  if (details->source_frame_id == 0)
    return;
  ExtensionWebNavigationTabObserver* tab_observer =
      ExtensionWebNavigationTabObserver::Get(details->source_tab_contents);
  if (!tab_observer) {
    NOTREACHED();
    return;
  }
  const FrameNavigationState& frame_navigation_state =
      tab_observer->frame_navigation_state();

  // If the TabContents was created as a response to an IPC from a renderer, it
  // doesn't yet have a wrapper, and we need to delay the extension event until
  // the TabContents is fully initialized.
  if (TabContentsWrapper::GetCurrentWrapperForContents(
      details->target_tab_contents) == NULL) {
    pending_tab_contents_[details->target_tab_contents] =
        PendingTabContents(
            details->source_tab_contents,
            details->source_frame_id,
            frame_navigation_state.IsMainFrame(details->source_frame_id),
            details->target_tab_contents,
            details->target_url);
  } else {
    DispatchOnBeforeRetarget(
        details->source_tab_contents,
        details->target_tab_contents->profile(),
        details->source_frame_id,
        frame_navigation_state.IsMainFrame(details->source_frame_id),
        details->target_tab_contents,
        details->target_url);
  }
}

void ExtensionWebNavigationEventRouter::TabAdded(TabContents* tab_contents) {
  std::map<TabContents*, PendingTabContents>::iterator iter =
      pending_tab_contents_.find(tab_contents);
  if (iter == pending_tab_contents_.end())
    return;

  DispatchOnBeforeRetarget(iter->second.source_tab_contents,
                           iter->second.target_tab_contents->profile(),
                           iter->second.source_frame_id,
                           iter->second.source_frame_is_main_frame,
                           iter->second.target_tab_contents,
                           iter->second.target_url);
  pending_tab_contents_.erase(iter);
}

void ExtensionWebNavigationEventRouter::TabDestroyed(
    TabContents* tab_contents) {
  pending_tab_contents_.erase(tab_contents);
  for (std::map<TabContents*, PendingTabContents>::iterator i =
           pending_tab_contents_.begin(); i != pending_tab_contents_.end(); ) {
    if (i->second.source_tab_contents == tab_contents)
      pending_tab_contents_.erase(i++);
    else
      ++i;
  }
}

// ExtensionWebNavigationTabObserver ------------------------------------------

ExtensionWebNavigationTabObserver::ExtensionWebNavigationTabObserver(
    TabContents* tab_contents)
    : TabContentsObserver(tab_contents) {
  g_tab_observer.Get().insert(TabObserverMap::value_type(tab_contents, this));
}

ExtensionWebNavigationTabObserver::~ExtensionWebNavigationTabObserver() {}

// static
ExtensionWebNavigationTabObserver* ExtensionWebNavigationTabObserver::Get(
    TabContents* tab_contents) {
  TabObserverMap::iterator i = g_tab_observer.Get().find(tab_contents);
  return i == g_tab_observer.Get().end() ? NULL : i->second;
}

void ExtensionWebNavigationTabObserver::DidStartProvisionalLoadForFrame(
    int64 frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    bool is_error_page,
    RenderViewHost* render_view_host) {
  navigation_state_.TrackFrame(frame_id,
                               validated_url,
                               is_main_frame,
                               is_error_page);
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  DispatchOnBeforeNavigate(
      tab_contents(), frame_id, is_main_frame, validated_url);
}

void ExtensionWebNavigationTabObserver::DidCommitProvisionalLoadForFrame(
    int64 frame_id,
    bool is_main_frame,
    const GURL& url,
    PageTransition::Type transition_type) {
  if (!navigation_state_.CanSendEvents(frame_id))
    return;

  bool is_reference_fragment_navigation =
      IsReferenceFragmentNavigation(frame_id, url);

  // Update the URL as it might have changed.
  navigation_state_.TrackFrame(frame_id,
                               url,
                               is_main_frame,
                               false);

  // On reference fragment navigations, only a new navigation state is
  // committed. We need to catch this case and generate a full sequence
  // of events.
  if (is_reference_fragment_navigation) {
    NavigatedReferenceFragment(frame_id, is_main_frame, url, transition_type);
    return;
  }
  DispatchOnCommitted(
      tab_contents(), frame_id, is_main_frame, url, transition_type);
}

void ExtensionWebNavigationTabObserver::DidFailProvisionalLoad(
    int64 frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    int error_code) {
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  ListValue args;
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger(keys::kTabIdKey,
                   ExtensionTabUtil::GetTabId(tab_contents()));
  dict->SetString(keys::kUrlKey, validated_url.spec());
  dict->SetInteger(keys::kFrameIdKey, GetFrameId(is_main_frame, frame_id));
  dict->SetString(keys::kErrorKey,
                  std::string(net::ErrorToString(error_code)));
  dict->SetDouble(keys::kTimeStampKey, MilliSecondsFromTime(base::Time::Now()));
  args.Append(dict);

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  navigation_state_.ErrorOccurredInFrame(frame_id);
  DispatchEvent(tab_contents()->profile(), keys::kOnErrorOccurred, json_args);
}

void ExtensionWebNavigationTabObserver::DocumentLoadedInFrame(
    int64 frame_id) {
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  DispatchOnDOMContentLoaded(tab_contents(),
                             navigation_state_.GetUrl(frame_id),
                             navigation_state_.IsMainFrame(frame_id),
                             frame_id);
}

void ExtensionWebNavigationTabObserver::DidFinishLoad(
    int64 frame_id) {
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  DispatchOnCompleted(tab_contents(),
                      navigation_state_.GetUrl(frame_id),
                      navigation_state_.IsMainFrame(frame_id),
                      frame_id);
}

void ExtensionWebNavigationTabObserver::TabContentsDestroyed(
    TabContents* tab) {
  g_tab_observer.Get().erase(tab);
}

// See also NavigationController::IsURLInPageNavigation.
bool ExtensionWebNavigationTabObserver::IsReferenceFragmentNavigation(
    int64 frame_id,
    const GURL& url) {
  GURL existing_url = navigation_state_.GetUrl(frame_id);
  if (existing_url == url)
    return false;

  url_canon::Replacements<char> replacements;
  replacements.ClearRef();
  return existing_url.ReplaceComponents(replacements) ==
      url.ReplaceComponents(replacements);
}

void ExtensionWebNavigationTabObserver::NavigatedReferenceFragment(
    int64 frame_id,
    bool is_main_frame,
    const GURL& url,
    PageTransition::Type transition_type) {
  DispatchOnBeforeNavigate(tab_contents(),
                           frame_id,
                           is_main_frame,
                           url);
  DispatchOnCommitted(tab_contents(),
                      frame_id,
                      is_main_frame,
                      url,
                      transition_type);
  DispatchOnDOMContentLoaded(tab_contents(),
                             url,
                             is_main_frame,
                             frame_id);
  DispatchOnCompleted(tab_contents(),
                      url,
                      is_main_frame,
                      frame_id);
}

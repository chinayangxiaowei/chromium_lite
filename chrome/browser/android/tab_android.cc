// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/tab_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/debug/trace_event.h"
#include "chrome/browser/android/chrome_web_contents_delegate_android.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/sync/glue/synced_tab_delegate_android.h"
#include "chrome/browser/ui/android/content_settings/popup_blocked_infobar_delegate.h"
#include "chrome/browser/ui/android/context_menu_helper.h"
#include "chrome/browser/ui/android/infobars/infobar_container_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/ui/android/window_android_helper.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/browser/ui/tab_contents/core_tab_helper.h"
#include "chrome/browser/ui/tab_helpers.h"
#include "chrome/browser/ui/toolbar/toolbar_model_impl.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "jni/TabBase_jni.h"

TabAndroid* TabAndroid::FromWebContents(content::WebContents* web_contents) {
  CoreTabHelper* core_tab_helper = CoreTabHelper::FromWebContents(web_contents);
  if (!core_tab_helper)
    return NULL;

  CoreTabHelperDelegate* core_delegate = core_tab_helper->delegate();
  if (!core_delegate)
    return NULL;

  return static_cast<TabAndroid*>(core_delegate);
}

TabAndroid* TabAndroid::GetNativeTab(JNIEnv* env, jobject obj) {
  return reinterpret_cast<TabAndroid*>(Java_TabBase_getNativePtr(env, obj));
}

TabAndroid::TabAndroid(JNIEnv* env, jobject obj)
    : weak_java_tab_(env, obj),
      session_tab_id_(),
      synced_tab_delegate_(new browser_sync::SyncedTabDelegateAndroid(this)) {
  Java_TabBase_setNativePtr(env, obj, reinterpret_cast<intptr_t>(this));
}

TabAndroid::~TabAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return;

  Java_TabBase_clearNativePtr(env, obj.obj());
}

base::android::ScopedJavaLocalRef<jobject> TabAndroid::GetJavaObject() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return weak_java_tab_.get(env);
}

int TabAndroid::GetAndroidId() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return -1;
  return Java_TabBase_getId(env, obj.obj());
}

int TabAndroid::GetSyncId() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return 0;
  return Java_TabBase_getSyncId(env, obj.obj());
}

base::string16 TabAndroid::GetTitle() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return base::string16();
  return base::android::ConvertJavaStringToUTF16(
      Java_TabBase_getTitle(env, obj.obj()));
}

GURL TabAndroid::GetURL() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return GURL::EmptyGURL();
  return GURL(base::android::ConvertJavaStringToUTF8(
      Java_TabBase_getUrl(env, obj.obj())));
}

bool TabAndroid::RestoreIfNeeded() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return false;
  return Java_TabBase_restoreIfNeeded(env, obj.obj());
}

content::ContentViewCore* TabAndroid::GetContentViewCore() const {
  if (!web_contents())
    return NULL;

  return content::ContentViewCore::FromWebContents(web_contents());
}

Profile* TabAndroid::GetProfile() const {
  if (!web_contents())
    return NULL;

  return Profile::FromBrowserContext(web_contents()->GetBrowserContext());
}

browser_sync::SyncedTabDelegate* TabAndroid::GetSyncedTabDelegate() const {
  return synced_tab_delegate_.get();
}

void TabAndroid::SetSyncId(int sync_id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> obj = weak_java_tab_.get(env);
  if (obj.is_null())
    return;
  Java_TabBase_setSyncId(env, obj.obj(), sync_id);
}

void TabAndroid::HandlePopupNavigation(chrome::NavigateParams* params) {
  NOTIMPLEMENTED();
}

void TabAndroid::OnReceivedHttpAuthRequest(jobject auth_handler,
                                           const base::string16& host,
                                           const base::string16& realm) {
  NOTIMPLEMENTED();
}

void TabAndroid::AddShortcutToBookmark(const GURL& url,
                                       const base::string16& title,
                                       const SkBitmap& skbitmap,
                                       int r_value,
                                       int g_value,
                                       int b_value) {
  NOTREACHED();
}

void TabAndroid::EditBookmark(int64 node_id,
                              const base::string16& node_title,
                              bool is_folder,
                              bool is_partner_bookmark) {
  NOTREACHED();
}

void TabAndroid::OnNewTabPageReady() {
  NOTREACHED();
}

bool TabAndroid::ShouldWelcomePageLinkToTermsOfService() {
  NOTIMPLEMENTED();
  return false;
}

void TabAndroid::SwapTabContents(content::WebContents* old_contents,
                                 content::WebContents* new_contents,
                                 bool did_start_load,
                                 bool did_finish_load) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // We need to notify the native InfobarContainer so infobars can be swapped.
  InfoBarContainerAndroid* infobar_container =
      reinterpret_cast<InfoBarContainerAndroid*>(
          Java_TabBase_getNativeInfoBarContainer(
              env,
              weak_java_tab_.get(env).obj()));
  InfoBarService* new_infobar_service = new_contents ?
      InfoBarService::FromWebContents(new_contents) : NULL;
  infobar_container->ChangeInfoBarService(new_infobar_service);

  Java_TabBase_swapWebContents(
      env,
      weak_java_tab_.get(env).obj(),
      reinterpret_cast<intptr_t>(new_contents),
      did_start_load,
      did_finish_load);
}

void TabAndroid::Observe(int type,
                         const content::NotificationSource& source,
                         const content::NotificationDetails& details) {
  JNIEnv* env = base::android::AttachCurrentThread();
  switch (type) {
    case chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED: {
      TabSpecificContentSettings* settings =
          TabSpecificContentSettings::FromWebContents(web_contents());
      if (!settings->IsBlockageIndicated(CONTENT_SETTINGS_TYPE_POPUPS)) {
        // TODO(dfalcantara): Create an InfoBarDelegate to keep the
        // PopupBlockedInfoBar logic native-side instead of straddling the JNI
        // boundary.
        int num_popups = 0;
        PopupBlockerTabHelper* popup_blocker_helper =
            PopupBlockerTabHelper::FromWebContents(web_contents());
        if (popup_blocker_helper)
          num_popups = popup_blocker_helper->GetBlockedPopupsCount();

        if (num_popups > 0)
          PopupBlockedInfoBarDelegate::Create(web_contents(), num_popups);

        settings->SetBlockageHasBeenIndicated(CONTENT_SETTINGS_TYPE_POPUPS);
      }
      break;
    }
    case chrome::NOTIFICATION_FAVICON_UPDATED:
      Java_TabBase_onFaviconUpdated(env, weak_java_tab_.get(env).obj());
      break;
    case content::NOTIFICATION_NAV_ENTRY_CHANGED:
      Java_TabBase_onNavEntryChanged(env, weak_java_tab_.get(env).obj());
      break;
    default:
      NOTREACHED() << "Unexpected notification " << type;
      break;
  }
}

void TabAndroid::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

void TabAndroid::InitWebContents(JNIEnv* env,
                                 jobject obj,
                                 jboolean incognito,
                                 jobject jcontent_view_core,
                                 jobject jweb_contents_delegate,
                                 jobject jcontext_menu_populator) {
  content::ContentViewCore* content_view_core =
      content::ContentViewCore::GetNativeContentViewCore(env,
                                                         jcontent_view_core);
  DCHECK(content_view_core);
  DCHECK(content_view_core->GetWebContents());

  web_contents_.reset(content_view_core->GetWebContents());
  TabHelpers::AttachTabHelpers(web_contents_.get());

  session_tab_id_.set_id(
      SessionTabHelper::FromWebContents(web_contents())->session_id().id());
  ContextMenuHelper::FromWebContents(web_contents())->SetPopulator(
      jcontext_menu_populator);
  WindowAndroidHelper::FromWebContents(web_contents())->
      SetWindowAndroid(content_view_core->GetWindowAndroid());
  CoreTabHelper::FromWebContents(web_contents())->set_delegate(this);
  web_contents_delegate_.reset(
      new chrome::android::ChromeWebContentsDelegateAndroid(
          env, jweb_contents_delegate));
  web_contents_delegate_->LoadProgressChanged(web_contents(), 0);
  web_contents()->SetDelegate(web_contents_delegate_.get());

  notification_registrar_.Add(
      this,
      chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,
      content::Source<content::WebContents>(web_contents()));
  notification_registrar_.Add(
      this,
      chrome::NOTIFICATION_FAVICON_UPDATED,
      content::Source<content::WebContents>(web_contents()));
  notification_registrar_.Add(
      this,
      content::NOTIFICATION_NAV_ENTRY_CHANGED,
      content::Source<content::NavigationController>(
           &web_contents()->GetController()));

  synced_tab_delegate_->SetWebContents(web_contents());

  // Set the window ID if there is a valid TabModel.
  TabModel* model = TabModelList::GetTabModelWithProfile(GetProfile());
  if (model) {
    SessionID window_id;
    window_id.set_id(model->GetSessionId());

    SessionTabHelper* session_tab_helper =
        SessionTabHelper::FromWebContents(web_contents());
    session_tab_helper->SetWindowID(window_id);
  }

  // Verify that the WebContents this tab represents matches the expected
  // off the record state.
  CHECK_EQ(GetProfile()->IsOffTheRecord(), incognito);
}

void TabAndroid::DestroyWebContents(JNIEnv* env,
                                    jobject obj,
                                    jboolean delete_native) {
  DCHECK(web_contents());

  notification_registrar_.Remove(
      this,
      chrome::NOTIFICATION_WEB_CONTENT_SETTINGS_CHANGED,
      content::Source<content::WebContents>(web_contents()));
  notification_registrar_.Remove(
      this,
      chrome::NOTIFICATION_FAVICON_UPDATED,
      content::Source<content::WebContents>(web_contents()));
  notification_registrar_.Remove(
      this,
      content::NOTIFICATION_NAV_ENTRY_CHANGED,
      content::Source<content::NavigationController>(
           &web_contents()->GetController()));

  web_contents()->SetDelegate(NULL);

  if (delete_native) {
    web_contents_.reset();
    synced_tab_delegate_->ResetWebContents();
  } else {
    // Release the WebContents so it does not get deleted by the scoped_ptr.
    ignore_result(web_contents_.release());
  }
}

base::android::ScopedJavaLocalRef<jobject> TabAndroid::GetWebContents(
    JNIEnv* env,
    jobject obj) {
  if (!web_contents_.get())
    return base::android::ScopedJavaLocalRef<jobject>();
  return web_contents_->GetJavaWebContents();
}

base::android::ScopedJavaLocalRef<jobject> TabAndroid::GetProfileAndroid(
    JNIEnv* env,
    jobject obj) {
  Profile* profile = GetProfile();
  if (!profile)
    return base::android::ScopedJavaLocalRef<jobject>();
  ProfileAndroid* profile_android = ProfileAndroid::FromProfile(profile);
  if (!profile_android)
    return base::android::ScopedJavaLocalRef<jobject>();

  return profile_android->GetJavaObject();
}

ToolbarModel::SecurityLevel TabAndroid::GetSecurityLevel(JNIEnv* env,
                                                         jobject obj) {
  return ToolbarModelImpl::GetSecurityLevelForWebContents(web_contents());
}

void TabAndroid::SetActiveNavigationEntryTitleForUrl(JNIEnv* env,
                                                     jobject obj,
                                                     jstring jurl,
                                                     jstring jtitle) {
  DCHECK(web_contents());

  base::string16 title;
  if (jtitle)
    title = base::android::ConvertJavaStringToUTF16(env, jtitle);

  std::string url;
  if (jurl)
    url = base::android::ConvertJavaStringToUTF8(env, jurl);

  content::NavigationEntry* entry =
      web_contents()->GetController().GetVisibleEntry();
  if (entry && url == entry->GetVirtualURL().spec())
    entry->SetTitle(title);
}

bool TabAndroid::Print(JNIEnv* env, jobject obj) {
  if (!web_contents())
    return false;

  printing::PrintViewManagerBasic::CreateForWebContents(web_contents());
  printing::PrintViewManagerBasic* print_view_manager =
      printing::PrintViewManagerBasic::FromWebContents(web_contents());
  if (print_view_manager == NULL)
    return false;

  print_view_manager->PrintNow();
  return true;
}

static void Init(JNIEnv* env, jobject obj) {
  TRACE_EVENT0("native", "TabAndroid::Init");
  // This will automatically bind to the Java object and pass ownership there.
  new TabAndroid(env, obj);
}

bool TabAndroid::RegisterTabAndroid(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

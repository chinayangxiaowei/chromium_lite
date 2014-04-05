// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_plugin_service_filter.h"

#include "base/logging.h"
#include "chrome/browser/plugin_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/browser/browser_thread.h"
#include "content/browser/plugin_service.h"
#include "content/browser/renderer_host/render_process_host.h"
#include "content/browser/resource_context.h"
#include "content/common/notification_service.h"
#include "webkit/plugins/npapi/plugin_list.h"

// static
ChromePluginServiceFilter* ChromePluginServiceFilter::GetInstance() {
  return Singleton<ChromePluginServiceFilter>::get();
}

void ChromePluginServiceFilter::RegisterResourceContext(
    PluginPrefs* plugin_prefs,
    const void* context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  base::AutoLock lock(lock_);
  resource_context_map_[context] = plugin_prefs;
}

void ChromePluginServiceFilter::UnregisterResourceContext(
    const void* context) {
  base::AutoLock lock(lock_);
  resource_context_map_.erase(context);
}

void ChromePluginServiceFilter::OverridePluginForTab(
    int render_process_id,
    int render_view_id,
    const GURL& url,
    const webkit::WebPluginInfo& plugin) {
  OverriddenPlugin overridden_plugin;
  overridden_plugin.render_process_id = render_process_id;
  overridden_plugin.render_view_id = render_view_id;
  overridden_plugin.url = url;
  overridden_plugin.plugin = plugin;
  base::AutoLock auto_lock(lock_);
  overridden_plugins_.push_back(overridden_plugin);
}

void ChromePluginServiceFilter::RestrictPluginToUrl(const FilePath& plugin_path,
                                                    const GURL& url) {
  base::AutoLock auto_lock(lock_);
  if (url.is_empty())
    restricted_plugins_.erase(plugin_path);
  else
    restricted_plugins_[plugin_path] = url;
}

bool ChromePluginServiceFilter::ShouldUsePlugin(
    int render_process_id,
    int render_view_id,
    const void* context,
    const GURL& url,
    const GURL& policy_url,
    webkit::WebPluginInfo* plugin) {
  base::AutoLock auto_lock(lock_);
  // Check whether the plugin is overridden.
  for (size_t i = 0; i < overridden_plugins_.size(); ++i) {
    if (overridden_plugins_[i].render_process_id == render_process_id &&
        overridden_plugins_[i].render_view_id == render_view_id &&
        (overridden_plugins_[i].url == url ||
         overridden_plugins_[i].url.is_empty())) {
      if (overridden_plugins_[i].plugin.path != plugin->path)
        return false;
      *plugin = overridden_plugins_[i].plugin;
      return true;
    }
  }

  // Check whether the plugin is disabled.
  ResourceContextMap::iterator prefs_it =
      resource_context_map_.find(context);
  if (prefs_it == resource_context_map_.end())
    return false;

  PluginPrefs* plugin_prefs = prefs_it->second.get();
  if (!plugin_prefs->IsPluginEnabled(*plugin))
    return false;

  // Check whether the plugin is restricted to a URL.
  RestrictedPluginMap::const_iterator it =
      restricted_plugins_.find(plugin->path);
  if (it != restricted_plugins_.end() &&
      (policy_url.scheme() != it->second.scheme() ||
       policy_url.host() != it->second.host())) {
    return false;
  }

  return true;
}

ChromePluginServiceFilter::ChromePluginServiceFilter() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CLOSED,
                 NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED,
                 NotificationService::AllSources());
}

ChromePluginServiceFilter::~ChromePluginServiceFilter() {
}

void ChromePluginServiceFilter::Observe(int type,
                                        const NotificationSource& source,
                                        const NotificationDetails& details) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CLOSED: {
      int render_process_id = Source<RenderProcessHost>(source).ptr()->id();

      base::AutoLock auto_lock(lock_);
      for (size_t i = 0; i < overridden_plugins_.size(); ++i) {
        if (overridden_plugins_[i].render_process_id == render_process_id) {
          overridden_plugins_.erase(overridden_plugins_.begin() + i);
          break;
        }
      }
      break;
    }
    case chrome::NOTIFICATION_PLUGIN_ENABLE_STATUS_CHANGED: {
      PluginService::GetInstance()->PurgePluginListCache(false);
      break;
    }
    default: {
      NOTREACHED();
    }
  }
}

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/frame_accessibility.h"

#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/browser_context.h"

namespace content {

// static
FrameAccessibility* FrameAccessibility::GetInstance() {
  return Singleton<FrameAccessibility>::get();
}

FrameAccessibility::ChildFrameMapping::ChildFrameMapping()
    : parent_frame_host(nullptr),
      accessibility_node_id(0),
      child_frame_tree_node_id(0),
      browser_plugin_instance_id(0) {}

FrameAccessibility::FrameAccessibility() {}

FrameAccessibility::~FrameAccessibility() {}

void FrameAccessibility::AddChildFrame(
    RenderFrameHostImpl* parent_frame_host,
    int accessibility_node_id,
    int64 child_frame_tree_node_id) {
  for (std::vector<ChildFrameMapping>::iterator iter = mappings_.begin();
       iter != mappings_.end();
       ++iter) {
    // TODO(dmazzoni): the renderer should keep track of these mappings
    // and clear an existing mapping before setting a new one, that would
    // be safer than just updating existing mappings. http://crbug.com/413464
    if (iter->parent_frame_host == parent_frame_host &&
        (iter->accessibility_node_id == accessibility_node_id ||
         iter->child_frame_tree_node_id == child_frame_tree_node_id)) {
      iter->accessibility_node_id = accessibility_node_id;
      iter->child_frame_tree_node_id = child_frame_tree_node_id;
      return;
    }
  }

  ChildFrameMapping new_mapping;
  new_mapping.parent_frame_host = parent_frame_host;
  new_mapping.accessibility_node_id = accessibility_node_id;
  new_mapping.child_frame_tree_node_id = child_frame_tree_node_id;
  mappings_.push_back(new_mapping);
}

void FrameAccessibility::AddGuestWebContents(
    RenderFrameHostImpl* parent_frame_host,
    int accessibility_node_id,
    int browser_plugin_instance_id) {
  for (std::vector<ChildFrameMapping>::iterator iter = mappings_.begin();
       iter != mappings_.end();
       ++iter) {
    // TODO(dmazzoni): the renderer should keep track of these mappings
    // and clear an existing mapping before setting a new one, that would
    // be safer than just updating existing mappings. http://crbug.com/413464
    if (iter->parent_frame_host == parent_frame_host &&
        (iter->accessibility_node_id == accessibility_node_id ||
         iter->browser_plugin_instance_id == browser_plugin_instance_id)) {
      iter->accessibility_node_id = accessibility_node_id;
      iter->browser_plugin_instance_id = browser_plugin_instance_id;
      return;
    }
  }

  ChildFrameMapping new_mapping;
  new_mapping.parent_frame_host = parent_frame_host;
  new_mapping.accessibility_node_id = accessibility_node_id;
  new_mapping.browser_plugin_instance_id = browser_plugin_instance_id;
  mappings_.push_back(new_mapping);
}

void FrameAccessibility::OnRenderFrameHostDestroyed(
    RenderFrameHostImpl* render_frame_host) {
  // Since the order doesn't matter, the fastest way to remove all items
  // with this render_frame_host is to iterate over the vector backwards,
  // swapping each one with the back element if we need to delete it.
  int initial_len = static_cast<int>(mappings_.size());
  for (int i = initial_len - 1; i >= 0; i--) {
    if (mappings_[i].parent_frame_host == render_frame_host) {
      mappings_[i] = mappings_.back();
      mappings_.pop_back();
    }
  }
}

RenderFrameHostImpl* FrameAccessibility::GetChild(
    RenderFrameHostImpl* parent_frame_host,
    int accessibility_node_id) {
  for (std::vector<ChildFrameMapping>::iterator iter = mappings_.begin();
       iter != mappings_.end();
       ++iter) {
    if (iter->parent_frame_host != parent_frame_host ||
        iter->accessibility_node_id != accessibility_node_id) {
      continue;
    }

    if (iter->child_frame_tree_node_id) {
      return GetRFHIFromFrameTreeNodeId(
          parent_frame_host, iter->child_frame_tree_node_id);
    }

    if (iter->browser_plugin_instance_id) {
      RenderFrameHost* guest =
          parent_frame_host->delegate()->GetGuestByInstanceID(
              iter->browser_plugin_instance_id);
      if (guest)
        return static_cast<RenderFrameHostImpl*>(guest);
    }
  }

  return nullptr;
}

void FrameAccessibility::GetAllChildFrames(
    RenderFrameHostImpl* parent_frame_host,
    std::vector<RenderFrameHostImpl*>* child_frame_hosts) {
  CHECK(child_frame_hosts);

  for (std::vector<ChildFrameMapping>::iterator iter = mappings_.begin();
       iter != mappings_.end();
       ++iter) {
    if (iter->parent_frame_host != parent_frame_host)
      continue;

    if (iter->child_frame_tree_node_id) {
      RenderFrameHostImpl* child_frame_host = GetRFHIFromFrameTreeNodeId(
          parent_frame_host, iter->child_frame_tree_node_id);
      if (child_frame_host)
        child_frame_hosts->push_back(child_frame_host);
    }

    if (iter->browser_plugin_instance_id) {
      RenderFrameHost* guest =
          parent_frame_host->delegate()->GetGuestByInstanceID(
              iter->browser_plugin_instance_id);
      if (guest)
        child_frame_hosts->push_back(static_cast<RenderFrameHostImpl*>(guest));
    }
  }
}

bool FrameAccessibility::GetParent(
    RenderFrameHostImpl* child_frame_host,
    RenderFrameHostImpl** out_parent_frame_host,
    int* out_accessibility_node_id) {
  for (std::vector<ChildFrameMapping>::iterator iter = mappings_.begin();
       iter != mappings_.end();
       ++iter) {
    if (iter->child_frame_tree_node_id) {
      FrameTreeNode* child_node =
          FrameTree::GloballyFindByID(iter->child_frame_tree_node_id);
      if (child_node &&
          child_node->current_frame_host() == child_frame_host) {
        // We should have gotten a *direct* child of the parent frame.
        if (child_node->parent() !=
            iter->parent_frame_host->frame_tree_node()) {
          NOTREACHED();
          return false;
        }

        if (out_parent_frame_host)
          *out_parent_frame_host = iter->parent_frame_host;
        if (out_accessibility_node_id)
          *out_accessibility_node_id = iter->accessibility_node_id;
        return true;
      }
    }

    if (iter->browser_plugin_instance_id) {
      RenderFrameHost* guest =
          iter->parent_frame_host->delegate()->GetGuestByInstanceID(
              iter->browser_plugin_instance_id);
      if (guest == child_frame_host) {
        if (out_parent_frame_host)
          *out_parent_frame_host = iter->parent_frame_host;
        if (out_accessibility_node_id)
          *out_accessibility_node_id = iter->accessibility_node_id;
        return true;
      }
    }
  }

  return false;
}

RenderFrameHostImpl* FrameAccessibility::GetRFHIFromFrameTreeNodeId(
    RenderFrameHostImpl* parent_frame_host,
    int64 child_frame_tree_node_id) {
  FrameTreeNode* child_node =
      FrameTree::GloballyFindByID(child_frame_tree_node_id);
  if (!child_node)
    return nullptr;

  // We should have gotten a *direct* child of the parent frame.
  if (child_node->parent() != parent_frame_host->frame_tree_node()) {
    NOTREACHED();
    return nullptr;
  }

  return child_node->current_frame_host();
}

}  // namespace content

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cert_viewer', function() {
  'use strict';

  /**
   * Initialize the certificate viewer dialog by wiring up the close button,
   * substituting in translated strings and requesting certificate details.
   */
  function initialize() {
    $('close').onclick = function() {
      window.close();
    }
    cr.ui.decorate('tabbox', cr.ui.TabBox);

    initializeTree($('hierarchy'), showCertificateFields);
    initializeTree($('cert-fields'), showCertificateFieldValue);

    i18nTemplate.process(document, templateData);
    stripGtkAccessorKeys();
    var args = JSON.parse(chrome.dialogArguments);
    chrome.send('requestCertificateInfo', [args.cert]);
  }

  /**
   * Initialize a cr.ui.Tree object from a given element using the specified
   * change handler.
   * @param {HTMLElement} tree The HTMLElement to style as a tree.
   * @param {function()} handler Function to call when a node is selected.
   */
  function initializeTree(tree, handler) {
    cr.ui.decorate(tree, cr.ui.Tree);
    tree.detail = {payload: {}, children: {}};
    tree.addEventListener('change', handler);
  }

  /**
   * The tab name strings in the languages file have accessor keys indicated
   * by a preceding & sign. Strip these out for now.
   * @TODO(flackr) These accessor keys could be implemented with Javascript or
   *     translated strings could be added / modified to remove the & sign.
   */
  function stripGtkAccessorKeys() {
    var tabs = $('tabs').childNodes;
    for (var i = 0; i < tabs.length; i++) {
      tabs[i].textContent = tabs[i].textContent.replace('&','');
    }
  }

  /**
   * Expand all nodes of the given tree object.
   * @param {cr.ui.Tree} tree The tree object to expand all nodes on.
   */
  function revealTree(tree) {
    tree.expanded = true;
    for (var key in tree.detail.children) {
      revealTree(tree.detail.children[key]);
    }
  }

  /**
   * This function is called from certificate_viewer_ui.cc with the certificate
   * information. Display all returned information to the user.
   * @param {Object} certInfo Certificate information in named fields.
   */
  function getCertificateInfo(certInfo) {
    for (var key in certInfo.general) {
      $(key).textContent = certInfo.general[key];
    }
    createCertificateHierarchy(certInfo.hierarchy);
  }

  /**
   * This function populates the certificate hierarchy.
   * @param {Object} hierarchy A dictionary containing the hierarchy.
   */
  function createCertificateHierarchy(hierarchy) {
    var treeItem = $('hierarchy');
    treeItem.add(treeItem.detail.children['root'] =
        constructTree(hierarchy[0]));
    revealTree(treeItem);
  }

  /**
   * Constructs a cr.ui.TreeItem corresponding to the passed in tree
   * @param {Object} tree Dictionary describing the tree structure.
   * @return {cr.ui.TreeItem} Tree node corresponding to the input dictionary.
   */
  function constructTree(tree)
  {
    var treeItem = new cr.ui.TreeItem({
        label: tree.label,
        detail: {payload: tree.payload ? tree.payload : {},
            children: {}
        }});
    if (tree.children) {
      for (var i = 0; i < tree.children.length; i++) {
        treeItem.add(treeItem.detail.children[i] =
            constructTree(tree.children[i]));
      }
    }
    return treeItem;
  }

  /**
   * Show certificate fields for the selected certificate in the hierarchy.
   */
  function showCertificateFields() {
    var treeItem = $('cert-fields');
    for (var key in treeItem.detail.children) {
      treeItem.remove(treeItem.detail.children[key]);
      delete treeItem.detail.children[key];
    }
    var item = $('hierarchy').selectedItem;
    if (item && item.detail.payload.fields) {
      treeItem.add(treeItem.detail.children['root'] =
          constructTree(item.detail.payload.fields[0]));
      revealTree(treeItem);
    }
  }

  /**
   * Show certificate field value for a selected certificate field.
   */
  function showCertificateFieldValue() {
    var item = $('cert-fields').selectedItem;
    if (item && item.detail.payload.val)
      $('cert-field-value').textContent = item.detail.payload.val;
    else
      $('cert-field-value').textContent = '';
  }

  return {
    initialize: initialize,
    getCertificateInfo: getCertificateInfo,
  };
});

document.addEventListener('DOMContentLoaded', cert_viewer.initialize);

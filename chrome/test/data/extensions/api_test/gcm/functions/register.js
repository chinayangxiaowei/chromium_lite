// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  senderIds = ["Sender1", "Sender2"];
  chrome.gcm.register(senderIds, function(registrationId) {});
};

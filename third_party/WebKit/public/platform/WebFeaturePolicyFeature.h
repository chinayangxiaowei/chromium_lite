// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFeaturePolicyFeature_h
#define WebFeaturePolicyFeature_h

namespace blink {

// These values map to the features which can be controlled by Feature Policy.
//
// Features are defined in
// https://wicg.github.io/feature-policy/#defined-features. Many of these are
// still under development in blink behind the featurePolicyExperimentalFeatures
// flag, see getWebFeaturePolicyFeature().
enum class WebFeaturePolicyFeature {
  kNotFound = 0,
  // Controls access to video input devices.
  kCamera,
  // Controls whether navigator.requestMediaKeySystemAccess is allowed.
  kEme,
  // Controls whether Element.requestFullscreen is allowed.
  kFullscreen,
  // Controls access to Geolocation interface.
  kGeolocation,
  // Controls access to audio input devices.
  kMicrophone,
  // Controls access to requestMIDIAccess method.
  kMidiFeature,
  // Controls access to PaymentRequest interface.
  kPayment,
  // Controls access to audio output devices.
  kSpeaker,
  // Controls access to navigator.vibrate method.
  kVibrate,
  // Controls access to document.cookie attribute.
  kDocumentCookie,
  // Contols access to document.domain attribute.
  kDocumentDomain,
  // Controls access to document.write and document.writeln methods.
  kDocumentWrite,
  // Controls access to Notification interface.
  kNotifications,
  // Controls access to PushManager interface.
  kPush,
  // Controls whether synchronous script elements will run.
  kSyncScript,
  // Controls use of synchronous XMLHTTPRequest API.
  kSyncXHR,
  // Controls access to RTCPeerConnection interface.
  kWebRTC,
  // Controls access to the WebUSB API.
  kUsb,
  LAST_FEATURE = kUsb
};

}  // namespace blink

#endif  // WebFeaturePolicyFeature_h

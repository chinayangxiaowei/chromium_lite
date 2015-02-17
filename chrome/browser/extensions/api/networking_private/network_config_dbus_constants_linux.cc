// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/networking_private/network_config_dbus_constants_linux.h"

namespace extensions {

namespace networking_private {

// Network manager API strings.
const char kNetworkManagerPath[] = "/org/freedesktop/NetworkManager";
const char kNetworkManagerNamespace[] = "org.freedesktop.NetworkManager";
const char kNetworkManagerAccessPointNamespace[] =
    "org.freedesktop.NetworkManager.AccessPoint";
const char kNetworkManagerActiveConnectionNamespace[] =
    "org.freedesktop.NetworkManager.Connection.Active";
const char kNetworkManagerDeviceNamespace[] =
    "org.freedesktop.NetworkManager.Device";
const char kNetworkManagerWirelessDeviceNamespace[] =
    "org.freedesktop.NetworkManager.Device.Wireless";
const char kNetworkManagerActiveConnections[] = "ActiveConnections";
const char kNetworkManagerSpecificObject[] = "SpecificObject";
const char kNetworkManagerDeviceType[] = "DeviceType";
const char kNetworkManagerGetDevicesMethod[] = "GetDevices";
const char kNetworkManagerGetAccessPointsMethod[] = "GetAccessPoints";
const char kNetworkManagerAddAndActivateConnectionMethod[] =
    "AddAndActivateConnection";
const char kNetworkManagerGetMethod[] = "Get";
const char kNetworkManagerSsidProperty[] = "Ssid";
const char kNetworkManagerStrengthProperty[] = "Strength";
const char kNetworkManagerWpaFlagsProperty[] = "WpaFlags";

// Network manager connection configuration strings.
const char kNetworkManagerConnectionConfig80211Wireless[] = "802-11-wireless";
const char kNetworkManagerConnectionConfigSsid[] = "ssid";

}  // namespace networking_private

}  // namespace extensions

// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/content_settings_helper.h"

#include "base/utf_string_conversions.h"
#include "base/string_piece.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"

namespace content_settings_helper {

std::string OriginToString(const GURL& origin) {
   std::string port_component(origin.IntPort() != url_parse::PORT_UNSPECIFIED ?
      ":" + origin.port() : "");
  std::string scheme_component(!origin.SchemeIs(chrome::kHttpScheme) ?
      origin.scheme() + chrome::kStandardSchemeSeparator : "");
  return scheme_component + origin.host() + port_component;
}

std::wstring OriginToWString(const GURL& origin) {
  return UTF8ToWide(OriginToString(origin));
}

}  // namespace content_settings_helper

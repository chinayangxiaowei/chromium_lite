// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/PagePopupClient.h"

#include "testing/gtest/include/gtest/gtest.h"
#include <string>

namespace blink {

TEST(PagePopupClientTest, AddJavaScriptString) {
  RefPtr<SharedBuffer> buffer = SharedBuffer::Create();
  PagePopupClient::AddJavaScriptString(
      String::FromUTF8("abc\r\n'\"</script>\t\f\v\xE2\x80\xA8\xE2\x80\xA9"),
      buffer.Get());
  EXPECT_EQ(
      "\"abc\\r\\n'\\\"\\x3C/script>\\u0009\\u000C\\u000B\\u2028\\u2029\"",
      std::string(buffer->Data(), buffer->size()));
}

}  // namespace blink

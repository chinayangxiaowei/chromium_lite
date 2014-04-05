// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/file_util.h"
#include "base/path_service.h"
#include "base/string_number_conversions.h"
#include "base/test/test_timeouts.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/automation/tab_proxy.h"
#include "chrome/test/ui/javascript_test_util.h"
#include "chrome/test/ui/ui_perf_test.h"
#include "net/base/net_util.h"

namespace {

class FrameRateTest : public UIPerfTest {
 public:
  FrameRateTest() {
    show_window_ = true;
    dom_automation_enabled_ = true;
  }

  virtual FilePath GetDataPath(const std::string& name) {
    // Make sure the test data is checked out.
    FilePath test_path;
    PathService::Get(chrome::DIR_TEST_DATA, &test_path);
    test_path = test_path.Append(FILE_PATH_LITERAL("perf"));
    test_path = test_path.Append(FILE_PATH_LITERAL("frame_rate"));
    test_path = test_path.Append(FILE_PATH_LITERAL("content"));
    test_path = test_path.AppendASCII(name);
    return test_path;
  }

  void RunTest(const std::string& name, const std::string& suffix) {
    FilePath test_path = GetDataPath(name);
    ASSERT_TRUE(file_util::DirectoryExists(test_path))
        << "Missing test directory: " << test_path.value();

    test_path = test_path.Append(FILE_PATH_LITERAL("test.html"));

    scoped_refptr<TabProxy> tab(GetActiveTab());
    ASSERT_TRUE(tab.get());

    ASSERT_EQ(AUTOMATION_MSG_NAVIGATION_SUCCESS,
              tab->NavigateToURL(net::FilePathToFileURL(test_path)));

    // Start the tests.
    ASSERT_TRUE(tab->NavigateToURLAsync(GURL("javascript:__start_all();")));

    // Block until the tests completes.
    ASSERT_TRUE(WaitUntilJavaScriptCondition(
        tab, L"", L"window.domAutomationController.send(!__running_all);",
        TestTimeouts::huge_test_timeout_ms()));

    // Read out the results.
    std::wstring json;
    ASSERT_TRUE(tab->ExecuteAndExtractString(
        L"",
        L"window.domAutomationController.send("
        L"JSON.stringify(__calc_results_total()));",
        &json));

    std::map<std::string, std::string> results;
    ASSERT_TRUE(JsonDictionaryToMap(WideToUTF8(json), &results));

    ASSERT_TRUE(results.find("mean") != results.end());
    ASSERT_TRUE(results.find("sigma") != results.end());
    ASSERT_TRUE(results.find("gestures") != results.end());
    ASSERT_TRUE(results.find("means") != results.end());
    ASSERT_TRUE(results.find("sigmas") != results.end());

    std::cout << "Gestures: " + results["gestures"] + "\n";
    std::cout << "Means:    " + results["means"] + "\n";
    std::cout << "Sigmas:   " + results["sigmas"] + "\n";
    PrintResultList("fps", suffix, "", results["means"],
                    "frames-per-second", true);

    std::string mean_and_error = results["mean"] + "," + results["sigma"];
    PrintResultMeanAndError("fps", suffix, "", mean_and_error,
                            "frames-per-second", true);
  }
};

class FrameRateTest_Reference : public FrameRateTest {
 public:
  void SetUp() {
    UseReferenceBuild();
    FrameRateTest::SetUp();
  }
};

#define FRAME_RATE_TEST(content) \
TEST_F(FrameRateTest, content) { \
  RunTest(#content, ""); \
} \
TEST_F(FrameRateTest_Reference, content) { \
  RunTest(#content, "_ref"); \
}
FRAME_RATE_TEST(blank);
FRAME_RATE_TEST(googleblog);

}  // namespace

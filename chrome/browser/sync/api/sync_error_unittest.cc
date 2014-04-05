// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/api/sync_error.h"

#include <string>

#include "base/tracked.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::string;
using syncable::ModelType;

namespace {

typedef testing::Test SyncErrorTest;

TEST_F(SyncErrorTest, Unset) {
  SyncError error;
  EXPECT_FALSE(error.IsSet());
}

TEST_F(SyncErrorTest, Default) {
  tracked_objects::Location location = FROM_HERE;
  std::string msg = "test";
  ModelType type = syncable::PREFERENCES;
  SyncError error(location, msg, type);
  EXPECT_TRUE(error.IsSet());
  EXPECT_EQ(location.line_number(), error.location().line_number());
  EXPECT_EQ(msg, error.message());
  EXPECT_EQ(type, error.type());
}

TEST_F(SyncErrorTest, Reset) {
  tracked_objects::Location location = FROM_HERE;
  std::string msg = "test";
  ModelType type = syncable::PREFERENCES;

  SyncError error;
  EXPECT_FALSE(error.IsSet());

  error.Reset(location, msg, type);
  EXPECT_TRUE(error.IsSet());
  EXPECT_EQ(location.line_number(), error.location().line_number());
  EXPECT_EQ(msg, error.message());
  EXPECT_EQ(type, error.type());

  tracked_objects::Location location2 = FROM_HERE;
  std::string msg2 = "test";
  ModelType type2 = syncable::PREFERENCES;
  error.Reset(location2, msg2, type2);
  EXPECT_TRUE(error.IsSet());
  EXPECT_EQ(location2.line_number(), error.location().line_number());
  EXPECT_EQ(msg2, error.message());
  EXPECT_EQ(type2, error.type());
}

TEST_F(SyncErrorTest, Copy) {
  tracked_objects::Location location = FROM_HERE;
  std::string msg = "test";
  ModelType type = syncable::PREFERENCES;

  SyncError error1;
  EXPECT_FALSE(error1.IsSet());
  SyncError error2(error1);
  EXPECT_FALSE(error2.IsSet());

  error1.Reset(location, msg, type);
  EXPECT_TRUE(error1.IsSet());
  EXPECT_EQ(location.line_number(), error1.location().line_number());
  EXPECT_EQ(msg, error1.message());
  EXPECT_EQ(type, error1.type());

  SyncError error3(error1);
  EXPECT_TRUE(error3.IsSet());
  EXPECT_EQ(error1.location().line_number(), error3.location().line_number());
  EXPECT_EQ(error1.message(), error3.message());
  EXPECT_EQ(error1.type(), error3.type());

  SyncError error4;
  EXPECT_FALSE(error4.IsSet());
  SyncError error5(error4);
  EXPECT_FALSE(error5.IsSet());
}

TEST_F(SyncErrorTest, Assign) {
  tracked_objects::Location location = FROM_HERE;
  std::string msg = "test";
  ModelType type = syncable::PREFERENCES;

  SyncError error1;
  EXPECT_FALSE(error1.IsSet());
  SyncError error2;
  error2 = error1;
  EXPECT_FALSE(error2.IsSet());

  error1.Reset(location, msg, type);
  EXPECT_TRUE(error1.IsSet());
  EXPECT_EQ(location.line_number(), error1.location().line_number());
  EXPECT_EQ(msg, error1.message());
  EXPECT_EQ(type, error1.type());

  error2 = error1;
  EXPECT_TRUE(error2.IsSet());
  EXPECT_EQ(error1.location().line_number(), error2.location().line_number());
  EXPECT_EQ(error1.message(), error2.message());
  EXPECT_EQ(error1.type(), error2.type());

  error2 = SyncError();
  EXPECT_FALSE(error2.IsSet());
}

}  // namespace

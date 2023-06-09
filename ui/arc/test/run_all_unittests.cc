// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "mojo/edk/embedder/embedder.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace {

class UiArcTestSuite : public base::TestSuite {
 public:
  UiArcTestSuite(int argc, char** argv) : base::TestSuite(argc, argv) {}

 protected:
  void Initialize() override {
    base::TestSuite::Initialize();
    gl::GLSurfaceTestSupport::InitializeOneOff();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UiArcTestSuite);
};

}  // namespace

int main(int argc, char** argv) {
  UiArcTestSuite test_suite(argc, argv);

  mojo::edk::Init();
  return base::LaunchUnitTests(
      argc, argv,
      base::Bind(&UiArcTestSuite::Run, base::Unretained(&test_suite)));
}

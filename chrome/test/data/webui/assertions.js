// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function WebUIAssertionsTest() {}

WebUIAssertionsTest.prototype = {
  __proto__: testing.Test.prototype,
  browsePreload: 'chrome://DummyURL',
  typedefCppFixture: null,
};

GEN('#include "chrome/test/data//webui/assertions-inl.h"');
GEN('');
GEN('WebUIAssertionsTest::WebUIAssertionsTest() {}');
GEN('WebUIAssertionsTest::~WebUIAssertionsTest() {}');
GEN('MockWebUIProvider::MockWebUIProvider() {}');
GEN('MockWebUIProvider::~MockWebUIProvider() {}');

function testTwoExpects() {
  expectTrue(false);
  expectTrue(0);
}

TEST_F('WebUIAssertionsTest', 'testTwoExpects', function() {
  var result = runTest(testTwoExpects, []);

  expectFalse(result[0]);
  expectTrue(!!result[1].match(/expectTrue\(false\): false/));
  expectTrue(!!result[1].match(/expectTrue\(0\): 0/));
});

function twoExpects() {
  expectTrue(false, 'message1');
  expectTrue(false, 'message2');
}

function testCallTestTwice() {
  twoExpects();
  twoExpects();
}

TEST_F('WebUIAssertionsTest', 'testCallTestTwice', function() {
  var result = runTest(testCallTestTwice, []);
  expectFalse(result[0]);
  expectEquals(2, result[1].match(
      /expectTrue\(false, 'message1'\): message1: false/g).length);
  expectEquals(2, result[1].match(
      /expectTrue\(false, 'message2'\): message2: false/g).length);
});

function testConstructMessage() {
  var message = 1 + ' ' + 2;
  assertTrue(false, message);
}

TEST_F('WebUIAssertionsTest', 'testConstructedMessage', function() {
  var result = runTest(testConstructMessage, []);
  expectEquals(
      1, result[1].match(/assertTrue\(false, message\): 1 2: false/g).length);
});

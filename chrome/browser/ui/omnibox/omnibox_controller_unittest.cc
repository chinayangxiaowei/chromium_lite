// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.h"
#include "chrome/test/base/testing_profile.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/omnibox_client.h"
#include "components/omnibox/browser/omnibox_controller.h"
#include "components/sessions/core/session_id.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TestOmniboxClient : public OmniboxClient {
 public:
  explicit TestOmniboxClient(Profile* profile)
      : profile_(profile),
        scheme_classifier_(profile) {}
  ~TestOmniboxClient() override {}

  // OmniboxClient:
  std::unique_ptr<AutocompleteProviderClient> CreateAutocompleteProviderClient()
      override {
    return base::MakeUnique<ChromeAutocompleteProviderClient>(profile_);
  }
  const SessionID& GetSessionID() const override { return session_id_; }
  const AutocompleteSchemeClassifier& GetSchemeClassifier() const override {
    return scheme_classifier_;
  }

 private:
  Profile* profile_;
  ChromeAutocompleteSchemeClassifier scheme_classifier_;
  SessionID session_id_;

  DISALLOW_COPY_AND_ASSIGN(TestOmniboxClient);
};

}  // namespace

class OmniboxControllerTest : public testing::Test {
 protected:
  OmniboxControllerTest();
  ~OmniboxControllerTest() override;

  void CreateController();
  void AssertProviders(int expected_providers);

  const AutocompleteController::Providers& GetAutocompleteProviders() const {
    return omnibox_controller_->autocomplete_controller()->providers();
  }

 private:
  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  std::unique_ptr<TestOmniboxClient> omnibox_client_;
  std::unique_ptr<OmniboxController> omnibox_controller_;

  DISALLOW_COPY_AND_ASSIGN(OmniboxControllerTest);
};

OmniboxControllerTest::OmniboxControllerTest() {
}

OmniboxControllerTest::~OmniboxControllerTest() {
}

void OmniboxControllerTest::CreateController() {
  DCHECK(omnibox_client_);
  omnibox_controller_.reset(new OmniboxController(NULL, omnibox_client_.get()));
}

// Checks that the list of autocomplete providers used by the OmniboxController
// matches the one in the |expected_providers| bit field.
void OmniboxControllerTest::AssertProviders(int expected_providers) {
  const AutocompleteController::Providers& providers =
      GetAutocompleteProviders();

  for (size_t i = 0; i < providers.size(); ++i) {
    // Ensure this is a provider we wanted.
    int type = providers[i]->type();
    ASSERT_TRUE(expected_providers & type);

    // Remove it from expectations so we fail if it's there twice.
    expected_providers &= ~type;
  }

  // Ensure we saw all the providers we expected.
  ASSERT_EQ(0, expected_providers);
}

void OmniboxControllerTest::SetUp() {
  omnibox_client_.reset(new TestOmniboxClient(&profile_));
}

void OmniboxControllerTest::TearDown() {
  omnibox_controller_.reset();
  omnibox_client_.reset();
}

TEST_F(OmniboxControllerTest, CheckDefaultAutocompleteProviders) {
  CreateController();
  // First collect the basic providers.
  int observed_providers = 0;
  const AutocompleteController::Providers& providers =
      GetAutocompleteProviders();
  for (size_t i = 0; i < providers.size(); ++i)
    observed_providers |= providers[i]->type();
  // Ensure we have at least one provider.
  ASSERT_NE(0, observed_providers);

  // Ensure instant extended includes all the provides in classic Chrome.
  int providers_with_instant_extended = observed_providers;
  // TODO(beaudoin): remove TYPE_SEARCH once it's no longer needed to pass
  // the Instant suggestion through via FinalizeInstantQuery.
  CreateController();
  AssertProviders(providers_with_instant_extended);
}

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/wizard_screen.h"
#include "chrome/browser/policy/cloud_policy_subsystem.h"
#include "chrome/browser/ui/webui/chromeos/enterprise_enrollment_ui.h"
#include "chrome/common/net/gaia/gaia_auth_fetcher.h"

namespace chromeos {

class ScreenObserver;
class EnterpriseEnrollmentScreenActor;

// The screen implementation that links the enterprise enrollment UI into the
// OOBE wizard.
class EnterpriseEnrollmentScreen
    : public WizardScreen,
      public EnterpriseEnrollmentUI::Controller,
      public GaiaAuthConsumer,
      public policy::CloudPolicySubsystem::Observer {
 public:
  EnterpriseEnrollmentScreen(ScreenObserver* observer,
                             EnterpriseEnrollmentScreenActor* actor);
  virtual ~EnterpriseEnrollmentScreen();

  // WizardScreen implementation:
  virtual void PrepareToShow() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;

  // EnterpriseEnrollmentUI::Controller implementation:
  virtual void OnAuthSubmitted(const std::string& user,
                               const std::string& password,
                               const std::string& captcha,
                               const std::string& access_code) OVERRIDE;
  virtual void OnAuthCancelled() OVERRIDE;
  virtual void OnConfirmationClosed() OVERRIDE;
  virtual bool GetInitialUser(std::string* user) OVERRIDE;

  // GaiaAuthConsumer implementation:
  virtual void OnClientLoginSuccess(const ClientLoginResult& result) OVERRIDE;
  virtual void OnClientLoginFailure(
      const GoogleServiceAuthError& error) OVERRIDE;

  virtual void OnIssueAuthTokenSuccess(const std::string& service,
                                       const std::string& auth_token) OVERRIDE;
  virtual void OnIssueAuthTokenFailure(
      const std::string& service,
      const GoogleServiceAuthError& error) OVERRIDE;

  // CloudPolicySubsystem::Observer implementation:
  virtual void OnPolicyStateChanged(
      policy::CloudPolicySubsystem::PolicySubsystemState state,
      policy::CloudPolicySubsystem::ErrorDetails error_details) OVERRIDE;

 private:
  void HandleAuthError(const GoogleServiceAuthError& error);

  // Starts the Lockbox storage process.
  void WriteInstallAttributesData();

  EnterpriseEnrollmentScreenActor* actor_;
  bool is_showing_;
  scoped_ptr<GaiaAuthFetcher> auth_fetcher_;
  std::string user_;
  std::string captcha_token_;
  scoped_ptr<policy::CloudPolicySubsystem::ObserverRegistrar> registrar_;
  ScopedRunnableMethodFactory<EnterpriseEnrollmentScreen>
      runnable_method_factory_;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseEnrollmentScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_H_

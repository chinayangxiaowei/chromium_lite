// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_ACTOR_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_ACTOR_H_

#include "base/basictypes.h"
#include "chrome/browser/ui/webui/chromeos/enterprise_enrollment_ui.h"

namespace chromeos {

// Interface class for the enterprise enrollment screen actor.
class EnterpriseEnrollmentScreenActor {
 public:
  virtual ~EnterpriseEnrollmentScreenActor() {}

  // Sets the controller.
  virtual void SetController(
      EnterpriseEnrollmentUI::Controller* controller) = 0;

  // Prepare the contents to showing.
  virtual void PrepareToShow() = 0;

  // Shows the contents of the screen.
  virtual void Show() = 0;

  // Hides the contents of the screen.
  virtual void Hide() = 0;

  // Sets whether the user is editable.
  virtual void SetEditableUser(bool editable) = 0;

  // Switches to the confirmation screen.
  virtual void ShowConfirmationScreen() = 0;

  // Show an authentication error.
  virtual void ShowAuthError(const GoogleServiceAuthError& error) = 0;
  virtual void ShowAccountError() = 0;
  virtual void ShowFatalAuthError() = 0;
  virtual void ShowFatalEnrollmentError() = 0;
  virtual void ShowNetworkEnrollmentError() = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_ENTERPRISE_ENROLLMENT_SCREEN_ACTOR_H_

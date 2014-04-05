// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/user_image_screen.h"

#include "base/compiler_specific.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/chromeos/login/default_user_images.h"
#include "chrome/browser/chromeos/login/login_utils.h"
#include "chrome/browser/chromeos/login/screen_observer.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/common/notification_service.h"
#include "grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace chromeos {

namespace {

// The resolution of the picture we want to get from the camera.
const int kFrameWidth = 480;
const int kFrameHeight = 480;

}  // namespace

UserImageScreen::UserImageScreen(ScreenObserver* screen_observer,
                                 UserImageScreenActor* actor)
    : WizardScreen(screen_observer),
      camera_controller_(this),
      actor_(actor) {
  actor_->SetDelegate(this);
  camera_controller_.set_frame_width(kFrameWidth);
  camera_controller_.set_frame_height(kFrameHeight);
  registrar_.Add(
      this,
      chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
      NotificationService::AllSources());
}

UserImageScreen::~UserImageScreen() {
  if (actor_)
    actor_->SetDelegate(NULL);
}

void UserImageScreen::PrepareToShow() {
  if (actor_)
    actor_->PrepareToShow();
}

void UserImageScreen::Show() {
  if (!actor_)
    return;

  actor_->Show();

  UserManager* user_manager = UserManager::Get();
  std::string logged_in_user = user_manager->logged_in_user().email();
  int selected_image_index =
      user_manager->GetUserDefaultImageIndex(logged_in_user);
  // The image must have been assigned by UserManager on new user login but
  // under some circumstances (i.e. the data is not written to Local State
  // or the file was corrupt) -1 could still be returned.
  if (selected_image_index == -1)
    selected_image_index = 0;
  actor_->SelectImage(selected_image_index);
}

void UserImageScreen::Hide() {
  camera_controller_.Stop();
  if (actor_)
    actor_->Hide();
}

void UserImageScreen::OnCaptureSuccess() {
  if (!actor_)
    return;

  SkBitmap frame;
  camera_controller_.GetFrame(&frame);
  if (!frame.isNull())
    actor_->UpdateVideoFrame(frame);
}

void UserImageScreen::OnCaptureFailure() {
  if (actor_)
    actor_->ShowCameraError();
}

void UserImageScreen::StartCamera() {
  if (!actor_)
    return;

  actor_->ShowCameraInitializing();
  camera_controller_.Start();
}

void UserImageScreen::StopCamera() {
  camera_controller_.Stop();
}

void UserImageScreen::OnPhotoTaken(const SkBitmap& image) {
  camera_controller_.Stop();

  UserManager* user_manager = UserManager::Get();
  DCHECK(user_manager);

  const UserManager::User& user = user_manager->logged_in_user();
  DCHECK(!user.email().empty());

  user_manager->SetLoggedInUserImage(image);
  user_manager->SaveUserImage(user.email(), image);
  get_screen_observer()->OnExit(ScreenObserver::USER_IMAGE_SELECTED);

  UMA_HISTOGRAM_ENUMERATION("UserImage.FirstTimeChoice",
                            kDefaultImagesCount,
                            kDefaultImagesCount + 1);
}

void UserImageScreen::OnDefaultImageSelected(int index) {
  camera_controller_.Stop();

  UserManager* user_manager = UserManager::Get();
  DCHECK(user_manager);

  const UserManager::User& user = user_manager->logged_in_user();
  DCHECK(!user.email().empty());

  const SkBitmap* image = ResourceBundle::GetSharedInstance().GetBitmapNamed(
      kDefaultImageResources[index]);
  user_manager->SetLoggedInUserImage(*image);
  user_manager->SaveUserImagePath(
      user.email(),
      GetDefaultImagePath(static_cast<size_t>(index)));
  get_screen_observer()->OnExit(ScreenObserver::USER_IMAGE_SELECTED);

  UMA_HISTOGRAM_ENUMERATION("UserImage.FirstTimeChoice",
                            index,
                            kDefaultImagesCount + 1);
}

void UserImageScreen::OnActorDestroyed(UserImageScreenActor* actor) {
  if (actor_ == actor)
    actor_ = NULL;
}

void UserImageScreen::Observe(int type,
                              const NotificationSource& source,
                              const NotificationDetails& details) {
  if (type != chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED)
    return;

  bool is_screen_locked = *Details<bool>(details).ptr();
  if (is_screen_locked)
    StopCamera();
  else if (actor_ && actor_->IsCapturing())
    StartCamera();
}

}  // namespace chromeos


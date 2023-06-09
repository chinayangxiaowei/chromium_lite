// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the
// services/resource_coordinator module.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_RESOURCE_COORDINATOR_FEATURES_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_RESOURCE_COORDINATOR_FEATURES_H_

#include "base/feature_list.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_export.h"

namespace features {

// The features should be documented alongside the definition of their values
// in the .cc file.
extern const SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_EXPORT base::Feature
    kGlobalResourceCoordinator;

}  // namespace features

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_RESOURCE_COORDINATOR_FEATURES_H_

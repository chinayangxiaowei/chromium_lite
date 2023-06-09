// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_RAPPOR_RECORDER_IMPL_H_
#define COMPONENTS_RAPPOR_RAPPOR_RECORDER_IMPL_H_

#include "base/threading/thread_checker.h"
#include "components/rappor/public/interfaces/rappor_recorder.mojom.h"

class GURL;

namespace service_manager {
struct BindSourceInfo;
}

namespace rappor {

class RapporServiceImpl;

// Records aggregate, privacy-preserving samples from the renderers.
// See https://www.chromium.org/developers/design-documents/rappor
class RapporRecorderImpl : public mojom::RapporRecorder {
 public:
  explicit RapporRecorderImpl(RapporServiceImpl* rappor_service);
  ~RapporRecorderImpl() override;

  static void Create(RapporServiceImpl* rappor_service,
                     const service_manager::BindSourceInfo& source_info,
                     mojom::RapporRecorderRequest request);

 private:
  // rappor::mojom::RapporRecorder:
  void RecordRappor(const std::string& metric,
                    const std::string& sample) override;
  void RecordRapporURL(const std::string& metric, const GURL& sample) override;

  RapporServiceImpl* rappor_service_;
  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(RapporRecorderImpl);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_RAPPOR_RECORDER_IMPL_H_

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_THUNK_PPB_GRAPHICS_3D_API_H_
#define PPAPI_THUNK_PPB_GRAPHICS_3D_API_H_

#include "ppapi/c/dev/ppb_graphics_3d_dev.h"
#include "ppapi/c/dev/ppb_graphics_3d_trusted_dev.h"

namespace ppapi {
namespace thunk {

class PPB_Graphics3D_API {
 public:
  virtual ~PPB_Graphics3D_API() {}

  // Graphics3D API.
  virtual int32_t GetAttribs(int32_t* attrib_list) = 0;
  virtual int32_t SetAttribs(int32_t* attrib_list) = 0;
  virtual int32_t SwapBuffers(PP_CompletionCallback callback) = 0;

  // Graphics3DTrusted API.
  virtual PP_Bool InitCommandBuffer(int32_t size) = 0;
  virtual PP_Bool GetRingBuffer(int* shm_handle,
                                uint32_t* shm_size) = 0;
  virtual PP_Graphics3DTrustedState GetState() = 0;
  virtual int32_t CreateTransferBuffer(uint32_t size) = 0;
  virtual PP_Bool DestroyTransferBuffer(int32_t id) = 0;
  virtual PP_Bool GetTransferBuffer(int32_t id,
                                    int* shm_handle,
                                    uint32_t* shm_size) = 0;
  virtual PP_Bool Flush(int32_t put_offset) = 0;
  virtual PP_Graphics3DTrustedState FlushSync(int32_t put_offset) = 0;
  virtual PP_Graphics3DTrustedState FlushSyncFast(int32_t put_offset,
                                                  int32_t last_known_get) = 0;

  // TODO(alokp): Implement GLESChromiumTextureMapping here after
  // deprecating Context3D.
};

}  // namespace thunk
}  // namespace ppapi

#endif  // PPAPI_THUNK_PPB_GRAPHICS_3D_API_H_

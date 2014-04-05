// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/shared_impl/graphics_3d_impl.h"

#include "base/logging.h"
#include "gpu/command_buffer/client/gles2_cmd_helper.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "ppapi/c/pp_errors.h"

namespace ppapi {

Graphics3DImpl::Graphics3DImpl()
    : transfer_buffer_id_(-1),
      swap_callback_(PP_BlockUntilComplete()) {
}

Graphics3DImpl::~Graphics3DImpl() {
  // Make sure that GLES2 implementation has already been destroyed.
  DCHECK_EQ(transfer_buffer_id_, -1);
  DCHECK(!gles2_helper_.get());
  DCHECK(!gles2_impl_.get());
}

int32_t Graphics3DImpl::GetAttribs(int32_t* attrib_list) {
  // TODO(alokp): Implement me.
  return PP_ERROR_FAILED;
}

int32_t Graphics3DImpl::SetAttribs(int32_t* attrib_list) {
  // TODO(alokp): Implement me.
  return PP_ERROR_FAILED;
}

int32_t Graphics3DImpl::SwapBuffers(PP_CompletionCallback callback) {
  if (!callback.func) {
    // Blocking SwapBuffers isn't supported (since we have to be on the main
    // thread).
    return PP_ERROR_BADARGUMENT;
  }

  if (HasPendingSwap()) {
    // Already a pending SwapBuffers that hasn't returned yet.
    return PP_ERROR_INPROGRESS;
  }

  swap_callback_ = callback;
  return DoSwapBuffers();
}

void Graphics3DImpl::SwapBuffersACK(int32_t pp_error) {
  DCHECK(HasPendingSwap());
  PP_RunAndClearCompletionCallback(&swap_callback_, pp_error);
}

bool Graphics3DImpl::CreateGLES2Impl(int32 command_buffer_size,
                                     int32 transfer_buffer_size) {
  gpu::CommandBuffer* command_buffer = GetCommandBuffer();
  DCHECK(command_buffer);

  // Create the GLES2 helper, which writes the command buffer protocol.
  gles2_helper_.reset(new gpu::gles2::GLES2CmdHelper(command_buffer));
  if (!gles2_helper_->Initialize(command_buffer_size))
    return false;

  // Create a transfer buffer used to copy resources between the renderer
  // process and the GPU process.
  transfer_buffer_id_ =
      command_buffer->CreateTransferBuffer(transfer_buffer_size, -1);
  if (transfer_buffer_id_ < 0)
    return false;

  // Map the buffer into the renderer process's address space.
  gpu::Buffer transfer_buffer =
      command_buffer->GetTransferBuffer(transfer_buffer_id_);
  if (!transfer_buffer.ptr)
    return false;

  // Create the object exposing the OpenGL API.
  gles2_impl_.reset(new gpu::gles2::GLES2Implementation(
      gles2_helper_.get(),
      transfer_buffer.size,
      transfer_buffer.ptr,
      transfer_buffer_id_,
      false,
      true));

  return true;
}

void Graphics3DImpl::DestroyGLES2Impl() {
  gles2_impl_.reset();

  if (transfer_buffer_id_ != -1) {
    gpu::CommandBuffer* command_buffer = GetCommandBuffer();
    DCHECK(command_buffer);
    command_buffer->DestroyTransferBuffer(transfer_buffer_id_);
    transfer_buffer_id_ = -1;
  }

  gles2_helper_.reset();
}

}  // namespace ppapi


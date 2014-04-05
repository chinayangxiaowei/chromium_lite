// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/ppb_graphics_3d_impl.h"

#include "base/message_loop.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "webkit/plugins/ppapi/plugin_module.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"

using ppapi::thunk::PPB_Graphics3D_API;

namespace webkit {
namespace ppapi {

namespace {
const int32 kCommandBufferSize = 1024 * 1024;
const int32 kTransferBufferSize = 1024 * 1024;

PP_Bool ShmToHandle(base::SharedMemory* shm,
                    size_t size,
                    int* shm_handle,
                    uint32_t* shm_size) {
  if (!shm || !shm_handle || !shm_size)
    return PP_FALSE;
#if defined(OS_POSIX)
  *shm_handle = shm->handle().fd;
#elif defined(OS_WIN)
  *shm_handle = reinterpret_cast<int>(shm->handle());
#else
  #error "Platform not supported."
#endif
  *shm_size = size;
  return PP_TRUE;
}

PP_Graphics3DTrustedState PPStateFromGPUState(
    const gpu::CommandBuffer::State& s) {
  PP_Graphics3DTrustedState state = {
      s.num_entries,
      s.get_offset,
      s.put_offset,
      s.token,
      static_cast<PPB_Graphics3DTrustedError>(s.error),
      s.generation
  };
  return state;
}
}  // namespace.

PPB_Graphics3D_Impl::PPB_Graphics3D_Impl(PluginInstance* instance)
    : Resource(instance),
      bound_to_instance_(false),
      commit_pending_(false),
      callback_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {
}

PPB_Graphics3D_Impl::~PPB_Graphics3D_Impl() {
  DestroyGLES2Impl();
}

// static
PP_Resource PPB_Graphics3D_Impl::Create(PluginInstance* instance,
                                        PP_Config3D_Dev config,
                                        PP_Resource share_context,
                                        const int32_t* attrib_list) {
  scoped_refptr<PPB_Graphics3D_Impl> graphics_3d(
      new PPB_Graphics3D_Impl(instance));

  if (!graphics_3d->Init(config, share_context, attrib_list))
    return 0;

  return graphics_3d->GetReference();
}

PP_Resource PPB_Graphics3D_Impl::CreateRaw(PluginInstance* instance,
                                           PP_Config3D_Dev config,
                                           PP_Resource share_context,
                                           const int32_t* attrib_list) {
  scoped_refptr<PPB_Graphics3D_Impl> graphics_3d(
      new PPB_Graphics3D_Impl(instance));

  if (!graphics_3d->InitRaw(config, share_context, attrib_list))
    return 0;

  return graphics_3d->GetReference();
}

PPB_Graphics3D_API* PPB_Graphics3D_Impl::AsPPB_Graphics3D_API() {
  return this;
}

PP_Bool PPB_Graphics3D_Impl::InitCommandBuffer(int32_t size) {
  return PP_FromBool(GetCommandBuffer()->Initialize(size));
}

PP_Bool PPB_Graphics3D_Impl::GetRingBuffer(int* shm_handle,
                                           uint32_t* shm_size) {
  gpu::Buffer buffer = GetCommandBuffer()->GetRingBuffer();
  return ShmToHandle(buffer.shared_memory, buffer.size, shm_handle, shm_size);
}

PP_Graphics3DTrustedState PPB_Graphics3D_Impl::GetState() {
  return PPStateFromGPUState(GetCommandBuffer()->GetState());
}

int32_t PPB_Graphics3D_Impl::CreateTransferBuffer(uint32_t size) {
  return GetCommandBuffer()->CreateTransferBuffer(size, -1);
}

PP_Bool PPB_Graphics3D_Impl::DestroyTransferBuffer(int32_t id) {
  GetCommandBuffer()->DestroyTransferBuffer(id);
  return PP_TRUE;
}

PP_Bool PPB_Graphics3D_Impl::GetTransferBuffer(int32_t id,
                                               int* shm_handle,
                                               uint32_t* shm_size) {
  gpu::Buffer buffer = GetCommandBuffer()->GetTransferBuffer(id);
  return ShmToHandle(buffer.shared_memory, buffer.size, shm_handle, shm_size);
}

PP_Bool PPB_Graphics3D_Impl::Flush(int32_t put_offset) {
  GetCommandBuffer()->Flush(put_offset);
  return PP_TRUE;
}

PP_Graphics3DTrustedState PPB_Graphics3D_Impl::FlushSync(int32_t put_offset) {
  gpu::CommandBuffer::State state = GetCommandBuffer()->GetState();
  return PPStateFromGPUState(
      GetCommandBuffer()->FlushSync(put_offset, state.get_offset));
}

PP_Graphics3DTrustedState PPB_Graphics3D_Impl::FlushSyncFast(
    int32_t put_offset,
    int32_t last_known_get) {
  return PPStateFromGPUState(
      GetCommandBuffer()->FlushSync(put_offset, last_known_get));
}

bool PPB_Graphics3D_Impl::BindToInstance(bool bind) {
  bound_to_instance_ = bind;
  if (bind && gles2_impl()) {
    // Resize the backing texture to the size of the instance when it is bound.
    // TODO(alokp): This should be the responsibility of plugins.
    const gfx::Size& size = instance()->position().size();
    gles2_impl()->ResizeCHROMIUM(size.width(), size.height());
  }
  return true;
}

unsigned int PPB_Graphics3D_Impl::GetBackingTextureId() {
  return platform_context_->GetBackingTextureId();
}

void PPB_Graphics3D_Impl::ViewInitiatedPaint() {
}

void PPB_Graphics3D_Impl::ViewFlushedPaint() {
  commit_pending_ = false;

  if (HasPendingSwap())
    SwapBuffersACK(PP_OK);
}

gpu::CommandBuffer* PPB_Graphics3D_Impl::GetCommandBuffer() {
  return platform_context_->GetCommandBuffer();
}

int32 PPB_Graphics3D_Impl::DoSwapBuffers() {
  // We do not have a GLES2 implementation when using an OOP proxy.
  // The plugin-side proxy is responsible for adding the SwapBuffers command
  // to the command buffer in that case.
  if (gles2_impl())
    gles2_impl()->SwapBuffers();

  return PP_OK_COMPLETIONPENDING;
}

bool PPB_Graphics3D_Impl::Init(PP_Config3D_Dev config,
                               PP_Resource share_context,
                               const int32_t* attrib_list) {
  if (!InitRaw(config, share_context, attrib_list))
    return false;

  gpu::CommandBuffer* command_buffer = GetCommandBuffer();
  if (!command_buffer->Initialize(kCommandBufferSize))
    return false;

  return CreateGLES2Impl(kCommandBufferSize, kTransferBufferSize);
}

bool PPB_Graphics3D_Impl::InitRaw(PP_Config3D_Dev config,
                                  PP_Resource share_context,
                                  const int32_t* attrib_list) {
  // TODO(alokp): Support shared context.
  DCHECK_EQ(0, share_context);
  if (share_context != 0)
    return 0;

  platform_context_.reset(instance()->CreateContext3D());
  if (!platform_context_.get())
    return false;

  if (!platform_context_->Init())
    return false;

  platform_context_->SetContextLostCallback(
      callback_factory_.NewCallback(&PPB_Graphics3D_Impl::OnContextLost));
  platform_context_->SetSwapBuffersCallback(
      callback_factory_.NewCallback(&PPB_Graphics3D_Impl::OnSwapBuffers));
  return true;
}

void PPB_Graphics3D_Impl::OnSwapBuffers() {
  if (bound_to_instance_) {
    // If we are bound to the instance, we need to ask the compositor
    // to commit our backing texture so that the graphics appears on the page.
    // When the backing texture will be committed we get notified via
    // ViewFlushedPaint().
    instance()->CommitBackingTexture();
    commit_pending_ = true;
  } else if (HasPendingSwap()) {
    // If we're off-screen, no need to trigger and wait for compositing.
    // Just send the swap-buffers ACK to the plugin immediately.
    commit_pending_ = false;
    SwapBuffersACK(PP_OK);
  }
}

void PPB_Graphics3D_Impl::OnContextLost() {
  if (bound_to_instance_)
    instance()->BindGraphics(instance()->pp_instance(), 0);

  // Send context lost to plugin. This may have been caused by a PPAPI call, so
  // avoid re-entering.
  MessageLoop::current()->PostTask(FROM_HERE, NewRunnableMethod(
      this, &PPB_Graphics3D_Impl::SendContextLost));
}

void PPB_Graphics3D_Impl::SendContextLost() {
  // By the time we run this, the instance may have been deleted, or in the
  // process of being deleted. Even in the latter case, we don't want to send a
  // callback after DidDestroy.
  if (!instance() || !instance()->container())
    return;

  const PPP_Graphics3D_Dev* ppp_graphics_3d =
      static_cast<const PPP_Graphics3D_Dev*>(
          instance()->module()->GetPluginInterface(
              PPP_GRAPHICS_3D_DEV_INTERFACE));
  if (ppp_graphics_3d)
    ppp_graphics_3d->Graphics3DContextLost(instance()->pp_instance());
}

}  // namespace ppapi
}  // namespace webkit

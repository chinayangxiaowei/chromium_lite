/* Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef PPAPI_C_DEV_PPB_VIDEO_DECODER_DEV_H_
#define PPAPI_C_DEV_PPB_VIDEO_DECODER_DEV_H_

#include "ppapi/c/dev/pp_video_dev.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_var.h"

#define PPB_VIDEODECODER_DEV_INTERFACE_0_14 "PPB_VideoDecoder(Dev);0.14"
#define PPB_VIDEODECODER_DEV_INTERFACE PPB_VIDEODECODER_DEV_INTERFACE_0_14

// Video decoder interface.
//
// Typical usage:
// - Use Create() to get a new PPB_VideoDecoder_Dev resource.
// - Call Initialize() to create the underlying resources in the GPU process and
//   configure the decoder there.
// - Call Decode() to decode some video data.
// - Receive ProvidePictureBuffers callback
//   - Supply the decoder with textures using AssignPictureBuffers.
// - Receive PictureReady callbacks
//   - Hand the textures back to the decoder using ReusePictureBuffer.
// - To signal EOS to the decoder: call Flush() and wait for NotifyFlushDone
//   callback.
// - To reset the decoder (e.g. to implement Seek): call Reset() and wait for
//   NotifyResetDone callback.
// - To tear down the decoder call Destroy().
//
// See PPP_VideoDecoder_Dev for the notifications the decoder may send the
// plugin.
struct PPB_VideoDecoder_Dev {
  // Creates a video decoder. Initialize() must be called afterwards to
  // set its configuration.
  //
  // Parameters:
  //   |instance| pointer to the plugin instance.
  //
  // The created decoder is returned as PP_Resource. 0 means failure.
  PP_Resource (*Create)(PP_Instance instance);

  // Tests whether |resource| is a video decoder created through Create
  // function of this interface.
  //
  // Parameters:
  //   |resource| is handle to resource to test.
  //
  // Returns true if is a video decoder, false otherwise.
  PP_Bool (*IsVideoDecoder)(PP_Resource resource);

  // Initializes the video decoder with requested configuration.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |context| the GL context in which decoding will happen. This should be a
  //   resource of type PPB_Context3D_Dev.
  //   |decoder_config| the configuration to use to initialize the decoder.
  //   |callback| called after initialization is complete.
  //
  // Returns an error code from pp_errors.h.
  int32_t (*Initialize)(PP_Resource video_decoder,
                        PP_Resource context,
                        const PP_VideoConfigElement* decoder_config,
                        struct PP_CompletionCallback callback);

  // Dispatches bitstream buffer to the decoder.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |bitstream_buffer| is the bitstream buffer that contains the input data.
  //   |callback| will be called when |bitstream_buffer| has been processed by
  //   the decoder.
  //
  // Returns an error code from pp_errors.h.
  int32_t (*Decode)(PP_Resource video_decoder,
                    const struct PP_VideoBitstreamBuffer_Dev* bitstream_buffer,
                    struct PP_CompletionCallback callback);

  // Provides the decoder with texture-backed picture buffers for video
  // decoding.
  //
  // This function should be called when the plugin has its
  // ProvidePictureBuffers method called.  The decoder will stall until it has
  // received all the buffers it's asked for.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |no_of_buffers| how many buffers are behind picture buffer pointer.
  //   |buffers| contains the reference to the picture buffer that was
  //   allocated.
  void (*AssignPictureBuffers)(
      PP_Resource video_decoder, uint32_t no_of_buffers,
      const struct PP_PictureBuffer_Dev* buffers);

  // Tells the decoder to reuse the given picture buffer. Typical use of this
  // function is to call from PictureReady callback to recycle picture buffer
  // back to the decoder after blitting the image so that decoder can use the
  // image for output again.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |picture_buffer_id| contains the id of the picture buffer that was
  //   processed.
  void (*ReusePictureBuffer)(PP_Resource video_decoder,
                             int32_t picture_buffer_id);

  // Flush input and output buffers in the decoder.  Any pending inputs are
  // decoded and pending outputs are delivered to the plugin.  Once done
  // flushing, the decoder will call |callback|.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |callback| is one-time callback that will be called once the flushing
  //   request has been completed.
  //
  // Returns an error code from pp_errors.h.
  int32_t (*Flush)(PP_Resource video_decoder,
                   struct PP_CompletionCallback callback);

  // Reset the decoder as quickly as possible.  Pending inputs and outputs are
  // dropped and the decoder is put back into a state ready to receive further
  // Decode() calls.  |callback| will be called when the reset is done.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  //   |callback| is one-time callback that will be called once the reset
  //   request has been completed.
  //
  // Returns an error code from pp_errors.h.
  int32_t (*Reset)(PP_Resource video_decoder,
                   struct PP_CompletionCallback callback);

  // Tear down the decoder as quickly as possible.  Pending inputs and outputs
  // are dropped and the decoder frees all of its resources.  Although resources
  // may be freed asynchronously, after this method returns no more callbacks
  // will be made on the client.  Any resources held by the client at that point
  // may be freed.
  //
  // Parameters:
  //   |video_decoder| is the previously created handle to the decoder resource.
  void (*Destroy)(PP_Resource video_decoder);
};

#endif  /* PPAPI_C_DEV_PPB_VIDEO_DECODER_DEV_H_ */

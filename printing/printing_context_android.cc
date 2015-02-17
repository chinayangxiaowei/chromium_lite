// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printing_context_android.h"

#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "jni/PrintingContext_jni.h"
#include "printing/metafile.h"
#include "printing/print_job_constants.h"
#include "printing/units.h"
#include "third_party/icu/source/i18n/unicode/ulocdata.h"

namespace {

// 1 inch in mils.
const int kInchToMil = 1000;

inline int Round(double x) {
  return static_cast<int>(x + 0.5);
}

// Sets the page sizes for a |PrintSettings| object.  |width| and |height|
// arguments should be in device units.
void SetSizes(
    printing::PrintSettings* settings, int dpi, int width, int height) {
  gfx::Size physical_size_device_units(width, height);
  // Assume full page is printable for now.
  gfx::Rect printable_area_device_units(0, 0, width, height);

  settings->set_dpi(dpi);
  settings->SetPrinterPrintableArea(physical_size_device_units,
                                    printable_area_device_units,
                                    false);
}

void GetPageRanges(JNIEnv* env,
                   jintArray int_arr,
                   printing::PageRanges& range_vector) {
  std::vector<int> pages;
  base::android::JavaIntArrayToIntVector(env, int_arr, &pages);
  for (std::vector<int>::const_iterator it = pages.begin();
      it != pages.end();
      ++it) {
    printing::PageRange range;
    range.from = *it;
    range.to = *it;
    range_vector.push_back(range);
  }
}

}  // namespace

namespace printing {

// static
scoped_ptr<PrintingContext> PrintingContext::Create(Delegate* delegate) {
  return make_scoped_ptr<PrintingContext>(new PrintingContextAndroid(delegate));
}

// static
void PrintingContextAndroid::PdfWritingDone(int fd, bool success) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PrintingContext_pdfWritingDone(env, fd, success);
}

PrintingContextAndroid::PrintingContextAndroid(Delegate* delegate)
    : PrintingContext(delegate) {
  // The constructor is run in the IO thread.
}

PrintingContextAndroid::~PrintingContextAndroid() {
}

void PrintingContextAndroid::AskUserForSettings(
    int max_pages,
    bool has_selection,
    const PrintSettingsCallback& callback) {
  // This method is always run in the UI thread.
  callback_ = callback;

  JNIEnv* env = base::android::AttachCurrentThread();
  if (j_printing_context_.is_null()) {
    j_printing_context_.Reset(Java_PrintingContext_create(
        env,
        reinterpret_cast<intptr_t>(this)));
  }

  Java_PrintingContext_pageCountEstimationDone(env,
                                               j_printing_context_.obj(),
                                               max_pages);
}

void PrintingContextAndroid::AskUserForSettingsReply(JNIEnv* env,
                                                     jobject obj,
                                                     jboolean success) {
  if (!success) {
    // TODO(cimamoglu): Differentiate between FAILED And CANCEL.
    callback_.Run(FAILED);
    return;
  }

  // We use device name variable to store the file descriptor.  This is hacky
  // but necessary. Since device name is not necessary for the upstream
  // printing code for Android, this is harmless.
  int fd = Java_PrintingContext_getFileDescriptor(env,
                                                  j_printing_context_.obj());
  settings_.set_device_name(base::IntToString16(fd));

  ScopedJavaLocalRef<jintArray> intArr =
      Java_PrintingContext_getPages(env, j_printing_context_.obj());
  if (intArr.obj() != NULL) {
    PageRanges range_vector;
    GetPageRanges(env, intArr.obj(), range_vector);
    settings_.set_ranges(range_vector);
  }

  int dpi = Java_PrintingContext_getDpi(env, j_printing_context_.obj());
  int width = Java_PrintingContext_getWidth(env, j_printing_context_.obj());
  int height = Java_PrintingContext_getHeight(env, j_printing_context_.obj());
  width = Round(ConvertUnitDouble(width, kInchToMil, 1.0) * dpi);
  height = Round(ConvertUnitDouble(height, kInchToMil, 1.0) * dpi);
  SetSizes(&settings_, dpi, width, height);

  callback_.Run(OK);
}

PrintingContext::Result PrintingContextAndroid::UseDefaultSettings() {
  DCHECK(!in_print_job_);

  ResetSettings();
  settings_.set_dpi(kDefaultPdfDpi);
  gfx::Size physical_size = GetPdfPaperSizeDeviceUnits();
  SetSizes(&settings_, kDefaultPdfDpi, physical_size.width(),
           physical_size.height());
  return OK;
}

gfx::Size PrintingContextAndroid::GetPdfPaperSizeDeviceUnits() {
  // NOTE: This implementation is the same as in PrintingContextNoSystemDialog.
  int32_t width = 0;
  int32_t height = 0;
  UErrorCode error = U_ZERO_ERROR;
  ulocdata_getPaperSize(
      delegate_->GetAppLocale().c_str(), &height, &width, &error);
  if (error > U_ZERO_ERROR) {
    // If the call failed, assume a paper size of 8.5 x 11 inches.
    LOG(WARNING) << "ulocdata_getPaperSize failed, using 8.5 x 11, error: "
                 << error;
    width = static_cast<int>(
        kLetterWidthInch * settings_.device_units_per_inch());
    height = static_cast<int>(
        kLetterHeightInch  * settings_.device_units_per_inch());
  } else {
    // ulocdata_getPaperSize returns the width and height in mm.
    // Convert this to pixels based on the dpi.
    float multiplier = 100 * settings_.device_units_per_inch();
    multiplier /= kHundrethsMMPerInch;
    width *= multiplier;
    height *= multiplier;
  }
  return gfx::Size(width, height);
}

PrintingContext::Result PrintingContextAndroid::UpdatePrinterSettings(
    bool external_preview,
    bool show_system_dialog) {
  DCHECK(!show_system_dialog);
  DCHECK(!in_print_job_);

  // Intentional No-op.

  return OK;
}

PrintingContext::Result PrintingContextAndroid::InitWithSettings(
    const PrintSettings& settings) {
  DCHECK(!in_print_job_);

  settings_ = settings;

  return OK;
}

PrintingContext::Result PrintingContextAndroid::NewDocument(
    const base::string16& document_name) {
  DCHECK(!in_print_job_);
  in_print_job_ = true;

  return OK;
}

PrintingContext::Result PrintingContextAndroid::NewPage() {
  if (abort_printing_)
    return CANCEL;
  DCHECK(in_print_job_);

  // Intentional No-op.

  return OK;
}

PrintingContext::Result PrintingContextAndroid::PageDone() {
  if (abort_printing_)
    return CANCEL;
  DCHECK(in_print_job_);

  // Intentional No-op.

  return OK;
}

PrintingContext::Result PrintingContextAndroid::DocumentDone() {
  if (abort_printing_)
    return CANCEL;
  DCHECK(in_print_job_);

  ResetSettings();
  return OK;
}

void PrintingContextAndroid::Cancel() {
  abort_printing_ = true;
  in_print_job_ = false;
}

void PrintingContextAndroid::ReleaseContext() {
  // Intentional No-op.
}

gfx::NativeDrawingContext PrintingContextAndroid::context() const {
  // Intentional No-op.
  return NULL;
}

// static
bool PrintingContextAndroid::RegisterPrintingContext(JNIEnv* env) {
   return RegisterNativesImpl(env);
}

}  // namespace printing

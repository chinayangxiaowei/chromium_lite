/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLImportChild_h
#define HTMLImportChild_h

#include "core/html/imports/HTMLImport.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"

namespace blink {

class V0CustomElementMicrotaskImportStep;
class HTMLImportLoader;
class HTMLImportChildClient;
class HTMLLinkElement;

//
// An import tree node subclass to encapsulate imported document
// lifecycle. This class is owned by HTMLImportsController. The actual loading
// is done by HTMLImportLoader, which can be shared among multiple
// HTMLImportChild of same link URL.
//
class HTMLImportChild final : public HTMLImport {
 public:
  HTMLImportChild(const KURL&, HTMLImportLoader*, SyncMode);
  ~HTMLImportChild() override;
  void Dispose();

  HTMLLinkElement* Link() const;
  const KURL& Url() const { return url_; }

  void OwnerInserted();
  void DidShareLoader();
  void DidStartLoading();

  // HTMLImport
  Document* GetDocument() const override;
  bool HasFinishedLoading() const override;
  HTMLImportLoader* Loader() const override;
  void StateWillChange() override;
  void StateDidChange() override;
  DECLARE_VIRTUAL_TRACE();

#if !defined(NDEBUG)
  void ShowThis() override;
#endif

  void SetClient(HTMLImportChildClient*);

  void DidFinishLoading();
  void DidFinishUpgradingCustomElements();
  void Normalize();

 private:
  void DidFinish();
  void ShareLoader();
  void CreateCustomElementMicrotaskStepIfNeeded();
  void InvalidateCustomElementMicrotaskStep();

  KURL url_;
  WeakMember<V0CustomElementMicrotaskImportStep> custom_element_microtask_step_;
  Member<HTMLImportLoader> loader_;
  Member<HTMLImportChildClient> client_;
};

inline HTMLImportChild* ToHTMLImportChild(HTMLImport* import) {
  DCHECK(!import || !import->IsRoot());
  return static_cast<HTMLImportChild*>(import);
}

}  // namespace blink

#endif  // HTMLImportChild_h

/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "modules/webdatabase/QuotaTracker.h"

#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/Threading.h"
#include "public/platform/Platform.h"
#include "public/platform/WebSecurityOrigin.h"

namespace blink {

QuotaTracker& QuotaTracker::Instance() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(QuotaTracker, tracker, new QuotaTracker);
  return tracker;
}

void QuotaTracker::GetDatabaseSizeAndSpaceAvailableToOrigin(
    SecurityOrigin* origin,
    const String& database_name,
    unsigned long long* database_size,
    unsigned long long* space_available) {
  // Extra scope to unlock prior to potentially calling Platform.
  {
    MutexLocker lock_data(data_guard_);
    DCHECK(database_sizes_.Contains(origin->ToRawString()));
    HashMap<String, SizeMap>::const_iterator it =
        database_sizes_.find(origin->ToRawString());
    DCHECK(it->value.Contains(database_name));
    *database_size = it->value.at(database_name);

    if (space_available_to_origins_.Contains(origin->ToRawString())) {
      *space_available = space_available_to_origins_.at(origin->ToRawString());
      return;
    }
  }

  // The embedder hasn't pushed this value to us, so we pull it as needed.
  *space_available = Platform::Current()->DatabaseGetSpaceAvailableForOrigin(
      WebSecurityOrigin(origin));
}

void QuotaTracker::UpdateDatabaseSize(SecurityOrigin* origin,
                                      const String& database_name,
                                      unsigned long long database_size) {
  MutexLocker lock_data(data_guard_);
  HashMap<String, SizeMap>::ValueType* it =
      database_sizes_.insert(origin->ToRawString(), SizeMap()).stored_value;
  it->value.Set(database_name, database_size);
}

void QuotaTracker::UpdateSpaceAvailableToOrigin(
    SecurityOrigin* origin,
    unsigned long long space_available) {
  MutexLocker lock_data(data_guard_);
  space_available_to_origins_.Set(origin->ToRawString(), space_available);
}

void QuotaTracker::ResetSpaceAvailableToOrigin(SecurityOrigin* origin) {
  MutexLocker lock_data(data_guard_);
  space_available_to_origins_.erase(origin->ToRawString());
}

}  // namespace blink

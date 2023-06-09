/*
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/timing/PerformanceUserTiming.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/timing/PerformanceBase.h"
#include "core/timing/PerformanceMark.h"
#include "core/timing/PerformanceMeasure.h"
#include "platform/Histogram.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/text/StringHash.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

using RestrictedKeyMap = HashMap<String, NavigationTimingFunction>;

RestrictedKeyMap* CreateRestrictedKeyMap() {
  RestrictedKeyMap* map = new RestrictedKeyMap();
  map->insert("navigationStart", &PerformanceTiming::navigationStart);
  map->insert("unloadEventStart", &PerformanceTiming::unloadEventStart);
  map->insert("unloadEventEnd", &PerformanceTiming::unloadEventEnd);
  map->insert("redirectStart", &PerformanceTiming::redirectStart);
  map->insert("redirectEnd", &PerformanceTiming::redirectEnd);
  map->insert("fetchStart", &PerformanceTiming::fetchStart);
  map->insert("domainLookupStart", &PerformanceTiming::domainLookupStart);
  map->insert("domainLookupEnd", &PerformanceTiming::domainLookupEnd);
  map->insert("connectStart", &PerformanceTiming::connectStart);
  map->insert("connectEnd", &PerformanceTiming::connectEnd);
  map->insert("secureConnectionStart",
              &PerformanceTiming::secureConnectionStart);
  map->insert("requestStart", &PerformanceTiming::requestStart);
  map->insert("responseStart", &PerformanceTiming::responseStart);
  map->insert("responseEnd", &PerformanceTiming::responseEnd);
  map->insert("domLoading", &PerformanceTiming::domLoading);
  map->insert("domInteractive", &PerformanceTiming::domInteractive);
  map->insert("domContentLoadedEventStart",
              &PerformanceTiming::domContentLoadedEventStart);
  map->insert("domContentLoadedEventEnd",
              &PerformanceTiming::domContentLoadedEventEnd);
  map->insert("domComplete", &PerformanceTiming::domComplete);
  map->insert("loadEventStart", &PerformanceTiming::loadEventStart);
  map->insert("loadEventEnd", &PerformanceTiming::loadEventEnd);
  return map;
}

const RestrictedKeyMap& GetRestrictedKeyMap() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(RestrictedKeyMap, map,
                                  CreateRestrictedKeyMap());
  return map;
}

}  // namespace

UserTiming::UserTiming(PerformanceBase& performance)
    : performance_(&performance) {}

static void InsertPerformanceEntry(PerformanceEntryMap& performance_entry_map,
                                   PerformanceEntry& entry) {
  PerformanceEntryMap::iterator it = performance_entry_map.find(entry.name());
  if (it != performance_entry_map.end()) {
    it->value.push_back(&entry);
  } else {
    PerformanceEntryVector vector(1);
    vector[0] = Member<PerformanceEntry>(entry);
    performance_entry_map.Set(entry.name(), vector);
  }
}

static void ClearPeformanceEntries(PerformanceEntryMap& performance_entry_map,
                                   const String& name) {
  if (name.IsNull()) {
    performance_entry_map.clear();
    return;
  }

  if (performance_entry_map.Contains(name))
    performance_entry_map.erase(name);
}

PerformanceEntry* UserTiming::Mark(const String& mark_name,
                                   ExceptionState& exception_state) {
  if (GetRestrictedKeyMap().Contains(mark_name)) {
    exception_state.ThrowDOMException(
        kSyntaxError, "'" + mark_name +
                          "' is part of the PerformanceTiming interface, and "
                          "cannot be used as a mark name.");
    return nullptr;
  }

  TRACE_EVENT_COPY_MARK("blink.user_timing", mark_name.Utf8().data());
  double start_time = performance_->now();
  PerformanceEntry* entry = PerformanceMark::Create(mark_name, start_time);
  InsertPerformanceEntry(marks_map_, *entry);
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      CustomCountHistogram, user_timing_mark_histogram,
      new CustomCountHistogram("PLT.UserTiming_Mark", 0, 600000, 100));
  user_timing_mark_histogram.Count(static_cast<int>(start_time));
  return entry;
}

void UserTiming::ClearMarks(const String& mark_name) {
  ClearPeformanceEntries(marks_map_, mark_name);
}

double UserTiming::FindExistingMarkStartTime(const String& mark_name,
                                             ExceptionState& exception_state) {
  if (marks_map_.Contains(mark_name))
    return marks_map_.at(mark_name).back()->startTime();

  if (GetRestrictedKeyMap().Contains(mark_name) && performance_->timing()) {
    double value = static_cast<double>(
        (performance_->timing()->*(GetRestrictedKeyMap().at(mark_name)))());
    if (!value) {
      exception_state.ThrowDOMException(
          kInvalidAccessError, "'" + mark_name +
                                   "' is empty: either the event hasn't "
                                   "happened yet, or it would provide "
                                   "cross-origin timing information.");
      return 0.0;
    }
    return value - performance_->timing()->navigationStart();
  }

  exception_state.ThrowDOMException(
      kSyntaxError, "The mark '" + mark_name + "' does not exist.");
  return 0.0;
}

PerformanceEntry* UserTiming::Measure(const String& measure_name,
                                      const String& start_mark,
                                      const String& end_mark,
                                      ExceptionState& exception_state) {
  double start_time = 0.0;
  double end_time = 0.0;

  if (start_mark.IsNull()) {
    end_time = performance_->now();
  } else if (end_mark.IsNull()) {
    end_time = performance_->now();
    start_time = FindExistingMarkStartTime(start_mark, exception_state);
    if (exception_state.HadException())
      return nullptr;
  } else {
    end_time = FindExistingMarkStartTime(end_mark, exception_state);
    if (exception_state.HadException())
      return nullptr;
    start_time = FindExistingMarkStartTime(start_mark, exception_state);
    if (exception_state.HadException())
      return nullptr;
  }

  // User timing events are stored as integer milliseconds from the start of
  // navigation, whereas trace events accept double seconds based off of
  // CurrentTime::monotonicallyIncreasingTime().
  double start_time_monotonic =
      performance_->TimeOrigin() + start_time / 1000.0;
  double end_time_monotonic = performance_->TimeOrigin() + end_time / 1000.0;

  TRACE_EVENT_COPY_NESTABLE_ASYNC_BEGIN_WITH_TIMESTAMP0(
      "blink.user_timing", measure_name.Utf8().data(),
      WTF::StringHash::GetHash(measure_name),
      TraceEvent::ToTraceTimestamp(start_time_monotonic));
  TRACE_EVENT_COPY_NESTABLE_ASYNC_END_WITH_TIMESTAMP0(
      "blink.user_timing", measure_name.Utf8().data(),
      WTF::StringHash::GetHash(measure_name),
      TraceEvent::ToTraceTimestamp(end_time_monotonic));

  PerformanceEntry* entry =
      PerformanceMeasure::Create(measure_name, start_time, end_time);
  InsertPerformanceEntry(measures_map_, *entry);
  if (end_time >= start_time) {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        CustomCountHistogram, measure_duration_histogram,
        new CustomCountHistogram("PLT.UserTiming_MeasureDuration", 0, 600000,
                                 100));
    measure_duration_histogram.Count(static_cast<int>(end_time - start_time));
  }
  return entry;
}

void UserTiming::ClearMeasures(const String& measure_name) {
  ClearPeformanceEntries(measures_map_, measure_name);
}

static PerformanceEntryVector ConvertToEntrySequence(
    const PerformanceEntryMap& performance_entry_map) {
  PerformanceEntryVector entries;

  for (const auto& entry : performance_entry_map)
    entries.AppendVector(entry.value);

  return entries;
}

static PerformanceEntryVector GetEntrySequenceByName(
    const PerformanceEntryMap& performance_entry_map,
    const String& name) {
  PerformanceEntryVector entries;

  PerformanceEntryMap::const_iterator it = performance_entry_map.find(name);
  if (it != performance_entry_map.end())
    entries.AppendVector(it->value);

  return entries;
}

PerformanceEntryVector UserTiming::GetMarks() const {
  return ConvertToEntrySequence(marks_map_);
}

PerformanceEntryVector UserTiming::GetMarks(const String& name) const {
  return GetEntrySequenceByName(marks_map_, name);
}

PerformanceEntryVector UserTiming::GetMeasures() const {
  return ConvertToEntrySequence(measures_map_);
}

PerformanceEntryVector UserTiming::GetMeasures(const String& name) const {
  return GetEntrySequenceByName(measures_map_, name);
}

DEFINE_TRACE(UserTiming) {
  visitor->Trace(performance_);
  visitor->Trace(marks_map_);
  visitor->Trace(measures_map_);
}

}  // namespace blink

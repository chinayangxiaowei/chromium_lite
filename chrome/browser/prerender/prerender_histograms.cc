// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_histograms.h"

#include <string>

#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/prerender/prerender_util.h"

namespace prerender {

namespace {

// Time window for which we will record windowed PLT's from the last
// observed link rel=prefetch tag.
const int kWindowDurationSeconds = 30;

std::string ComposeHistogramName(const std::string& prefix_type,
                                 const std::string& name) {
  if (prefix_type.empty())
    return std::string("Prerender.") + name;
  return std::string("Prerender.") + prefix_type + std::string("_") + name;
}

std::string GetHistogramName(Origin origin, uint8 experiment_id,
                             const std::string& name) {
  switch (origin) {
    case ORIGIN_OMNIBOX:
      if (experiment_id != kNoExperiment)
        return ComposeHistogramName("wash", name);
      return ComposeHistogramName("omnibox", name);
    case ORIGIN_LINK_REL_PRERENDER:
      if (experiment_id != kNoExperiment)
        return ComposeHistogramName("wash", name);
      return ComposeHistogramName("", name);
    case ORIGIN_GWS_PRERENDER:
      if (experiment_id == kNoExperiment)
        return ComposeHistogramName("gws", name);
      return ComposeHistogramName("exp" + std::string(1, experiment_id + '0'),
                                  name);
    default:
      NOTREACHED();
      break;
  };

  // Dummy return value to make the compiler happy.
  NOTREACHED();
  return ComposeHistogramName("wash", name);
}

}  // namespace

// Helper macros for experiment-based and origin-based histogram reporting.
#define PREFIXED_HISTOGRAM(histogram) \
  PREFIXED_HISTOGRAM_INTERNAL(GetCurrentOrigin(), GetCurrentExperimentId(), \
                              IsOriginExperimentWash(), histogram)

#define PREFIXED_HISTOGRAM_ORIGIN_EXPERIMENT(origin, experiment, histogram) \
  PREFIXED_HISTOGRAM_INTERNAL(origin, experiment, false, histogram)

#define PREFIXED_HISTOGRAM_INTERNAL(origin, experiment, wash, histogram) { \
  static uint8 recording_experiment = kNoExperiment; \
  if (recording_experiment == kNoExperiment && experiment != kNoExperiment) \
    recording_experiment = experiment; \
  if (wash) { \
    histogram; \
  } else if (experiment != kNoExperiment && \
             (origin != ORIGIN_GWS_PRERENDER || \
              experiment != recording_experiment)) { \
  } else if (origin == ORIGIN_LINK_REL_PRERENDER) { \
    histogram; \
  } else if (origin == ORIGIN_OMNIBOX) { \
    histogram; \
  } else if (experiment != kNoExperiment) { \
    histogram; \
  } else { \
    histogram; \
  } \
}

PrerenderHistograms::PrerenderHistograms()
    : last_experiment_id_(kNoExperiment),
      last_origin_(ORIGIN_LINK_REL_PRERENDER),
      origin_experiment_wash_(false) {
}

void PrerenderHistograms::RecordPrerender(Origin origin, const GURL& url) {
  // Check if we are doing an experiment.
  uint8 experiment = GetQueryStringBasedExperiment(url);

  // We need to update last_experiment_id_, last_origin_, and
  // origin_experiment_wash_.
  if (!WithinWindow()) {
    // If we are outside a window, this is a fresh start and we are fine,
    // and there is no mix.
    origin_experiment_wash_ = false;
  } else {
    // If we are inside the last window, there is a mish mash of origins
    // and experiments if either there was a mish mash before, or the current
    // experiment/origin does not match the previous one.
    if (experiment != last_experiment_id_ || origin != last_origin_)
      origin_experiment_wash_ = true;
  }

  last_origin_ = origin;
  last_experiment_id_ = experiment;

  // If we observe multiple tags within the 30 second window, we will still
  // reset the window to begin at the most recent occurrence, so that we will
  // always be in a window in the 30 seconds from each occurrence.
  last_prerender_seen_time_ = GetCurrentTimeTicks();
}

base::TimeTicks PrerenderHistograms::GetCurrentTimeTicks() const {
  return base::TimeTicks::Now();
}

// Helper macro for histograms.
#define RECORD_PLT(tag, perceived_page_load_time) { \
  PREFIXED_HISTOGRAM( \
    UMA_HISTOGRAM_CUSTOM_TIMES( \
        base::FieldTrial::MakeName(GetDefaultHistogramName(tag), "Prefetch"), \
        perceived_page_load_time, \
        base::TimeDelta::FromMilliseconds(10), \
        base::TimeDelta::FromSeconds(60), \
        100)); \
}

void PrerenderHistograms::RecordPerceivedPageLoadTime(
    base::TimeDelta perceived_page_load_time, bool was_prerender) const {
  bool within_window = WithinWindow();
  RECORD_PLT("PerceivedPLT", perceived_page_load_time);
  if (within_window)
    RECORD_PLT("PerceivedPLTWindowed", perceived_page_load_time);
  if (was_prerender) {
    RECORD_PLT("PerceivedPLTMatched", perceived_page_load_time);
  } else {
    if (within_window)
      RECORD_PLT("PerceivedPLTWindowNotMatched", perceived_page_load_time);
  }
}

bool PrerenderHistograms::WithinWindow() const {
  if (last_prerender_seen_time_.is_null())
    return false;
  base::TimeDelta elapsed_time =
      base::TimeTicks::Now() - last_prerender_seen_time_;
  return elapsed_time <= base::TimeDelta::FromSeconds(kWindowDurationSeconds);
}


void PrerenderHistograms::RecordTimeUntilUsed(
    base::TimeDelta time_until_used, base::TimeDelta max_age) const {
  PREFIXED_HISTOGRAM(UMA_HISTOGRAM_CUSTOM_TIMES(
      GetDefaultHistogramName("TimeUntilUsed"),
      time_until_used,
      base::TimeDelta::FromMilliseconds(10),
      max_age,
      50));
}

void PrerenderHistograms::RecordPerSessionCount(int count) const {
  PREFIXED_HISTOGRAM(UMA_HISTOGRAM_COUNTS(
      GetDefaultHistogramName("PrerendersPerSessionCount"), count));
}

void PrerenderHistograms::RecordTimeBetweenPrerenderRequests(
    base::TimeDelta time) const {
  PREFIXED_HISTOGRAM(UMA_HISTOGRAM_TIMES(
      GetDefaultHistogramName("TimeBetweenPrerenderRequests"), time));
}

void PrerenderHistograms::RecordFinalStatus(Origin origin,
                                            uint8 experiment_id,
                                            FinalStatus final_status) const {
  DCHECK(final_status != FINAL_STATUS_MAX);
  PREFIXED_HISTOGRAM_ORIGIN_EXPERIMENT(origin, experiment_id,
                     UMA_HISTOGRAM_ENUMERATION(
                         GetHistogramName(origin, experiment_id, "FinalStatus"),
                         final_status,
                         FINAL_STATUS_MAX));
}

std::string PrerenderHistograms::GetDefaultHistogramName(
    const std::string& name) const {
  if (!WithinWindow())
    return ComposeHistogramName("", name);
  if (origin_experiment_wash_)
    return ComposeHistogramName("wash", name);
  return GetHistogramName(last_origin_, last_experiment_id_, name);
}

uint8 PrerenderHistograms::GetCurrentExperimentId() const {
  if (!WithinWindow())
    return kNoExperiment;
  return last_experiment_id_;
}

Origin PrerenderHistograms::GetCurrentOrigin() const {
  if (!WithinWindow())
    return ORIGIN_LINK_REL_PRERENDER;
  return last_origin_;
}

bool PrerenderHistograms::IsOriginExperimentWash() const {
  if (!WithinWindow())
    return false;
  return origin_experiment_wash_;
}

}  // namespace prerender

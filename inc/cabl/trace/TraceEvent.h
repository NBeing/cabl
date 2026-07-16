/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstdint>

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

//! Chrome Trace Event Format phases we emit - a small subset (duration pairs,
//! instants, counters) is enough for Perfetto to render tracks, timing and
//! graphs without needing the full spec.
enum class TracePhase : char
{
  Begin = 'B',
  End = 'E',
  Instant = 'i',
  Counter = 'C',
};

//--------------------------------------------------------------------------------------------------

//! Fixed-size, non-allocating trace record. name/category are expected to be
//! string literals (or otherwise static-lifetime strings) - never freed,
//! never copied into an owned buffer, so recording one costs nothing beyond
//! a pointer store.
struct TraceEvent
{
  const char* name{nullptr};
  const char* category{nullptr};
  TracePhase phase{TracePhase::Instant};
  int64_t timestampUs{0};
  int32_t counterValue{0}; // only meaningful when phase == Counter
};

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

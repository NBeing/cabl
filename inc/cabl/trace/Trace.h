/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "cabl/trace/TraceRecorder.h"

//--------------------------------------------------------------------------------------------------

#if defined(CABL_TRACE_ENABLED)

namespace sl
{
namespace cabl
{

//! RAII helper for CABL_TRACE_SCOPE - records a Begin on construction and
//! the matching End on destruction.
class TraceScope
{
public:
  TraceScope(const char* category_, const char* name_) : m_category(category_), m_name(name_)
  {
    TraceRecorder::instance().recordBegin(m_category, m_name);
  }

  ~TraceScope()
  {
    TraceRecorder::instance().recordEnd(m_category, m_name);
  }

  TraceScope(const TraceScope&) = delete;
  TraceScope& operator=(const TraceScope&) = delete;

private:
  const char* m_category;
  const char* m_name;
};

} // namespace cabl
} // namespace sl

#define CABL_TRACE_CONCAT_INNER(a_, b_) a_##b_
#define CABL_TRACE_CONCAT(a_, b_) CABL_TRACE_CONCAT_INNER(a_, b_)

#define CABL_TRACE_THREAD_NAME(name_) sl::cabl::TraceRecorder::instance().registerThreadName(name_)
#define CABL_TRACE_INSTANT(cat_, name_) sl::cabl::TraceRecorder::instance().recordInstant(cat_, name_)
#define CABL_TRACE_COUNTER(cat_, name_, value_) \
  sl::cabl::TraceRecorder::instance().recordCounter(cat_, name_, value_)
#define CABL_TRACE_SCOPE(cat_, name_) \
  sl::cabl::TraceScope CABL_TRACE_CONCAT(cablTraceScope_, __LINE__)(cat_, name_)

#else // !CABL_TRACE_ENABLED

#define CABL_TRACE_THREAD_NAME(name_)
#define CABL_TRACE_INSTANT(cat_, name_)
#define CABL_TRACE_COUNTER(cat_, name_, value_)
#define CABL_TRACE_SCOPE(cat_, name_)

#endif // CABL_TRACE_ENABLED

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "cabl/trace/TraceEvent.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

//! Collects TraceEvents from any number of threads and dumps them as a
//! Chrome Trace Event Format JSON file, viewable at ui.perfetto.dev with no
//! server/install needed.
//!
//! Each thread gets its own fixed-capacity ring buffer (via a thread_local
//! cached pointer), so recording an event never contends with any other
//! thread - the only mutex-guarded section is the one-time-per-thread
//! buffer registration, never touched again on that thread's hot path
//! afterwards. This matters because the whole point of RTThreadRunner /
//! LockFreeQueue elsewhere in this codebase is to keep the MIDI thread free
//! of exactly this kind of contention - a tracer that puts a mutex in the
//! hot path would undo that.
//!
//! A dev/diagnostic tool, not a guarantee: buffers wrap on overflow
//! (oldest events silently overwritten). writeJson() is best-effort-safe to
//! call while other threads are still recording - it only reads an atomic
//! index plus plain-old-data fields, so at worst a slot mid-write shows up
//! stale or torn for one event - but for a clean, complete dump prefer
//! calling it only after every recording thread has been stopped and
//! joined. Note that in this codebase Coordinator is a process-wide
//! singleton (Coordinator::instance()), so its background thread - and any
//! Device's fast thread started via Device::startFastThread() - keeps
//! running for the life of the process regardless of any one Client's
//! lifetime; a still-running example's trace dump may have a slightly
//! incomplete tail from those threads.
class TraceRecorder
{
public:
  //! Per-thread ring buffer capacity. Sized for desktop dev use (~2.5MB per
  //! thread); this whole feature is meant to be compiled out entirely
  //! (see CABL_TRACE_ENABLED / cabl/trace/Trace.h) on memory-constrained
  //! targets like an ESP32-S3.
  static constexpr std::size_t kEventsPerThread = 65536;

  static TraceRecorder& instance()
  {
    static TraceRecorder inst;
    return inst;
  }

  TraceRecorder(const TraceRecorder&) = delete;
  TraceRecorder& operator=(const TraceRecorder&) = delete;

  //! Call once at the top of a thread's run function. Sets the friendly
  //! name Perfetto will show for that thread's track.
  void registerThreadName(const char* name_)
  {
    threadBuffer().name = name_;
  }

  void recordInstant(const char* category_, const char* name_)
  {
    record(category_, name_, TracePhase::Instant, 0);
  }

  void recordBegin(const char* category_, const char* name_)
  {
    record(category_, name_, TracePhase::Begin, 0);
  }

  void recordEnd(const char* category_, const char* name_)
  {
    record(category_, name_, TracePhase::End, 0);
  }

  void recordCounter(const char* category_, const char* name_, int32_t value_)
  {
    record(category_, name_, TracePhase::Counter, value_);
  }

  //! Best-effort snapshot - see the class comment above for what "safe" means
  //! here. Returns false if the file couldn't be opened.
  bool writeJson(const std::string& path_) const
  {
    std::vector<Row> rows = gatherSortedRows();

    std::vector<const ThreadBuffer*> buffers;
    {
      std::lock_guard<std::mutex> lock(m_registryMtx);
      for (const auto& buf : m_threadBuffers)
      {
        buffers.push_back(buf.get());
      }
    }

    std::FILE* file = std::fopen(path_.c_str(), "w");
    if (!file)
    {
      return false;
    }

    std::fputs("{\"traceEvents\":[\n", file);
    bool first = true;
    for (const auto& row : rows)
    {
      writeEvent(file, *row.event, row.tid, first);
      first = false;
    }
    for (std::size_t tid = 0; tid < buffers.size(); tid++)
    {
      writeThreadNameMeta(file, tid, buffers[tid]->name, first);
      first = false;
    }
    std::fputs("\n]}\n", file);
    std::fclose(file);
    return true;
  }

  //! Quick human-readable counts by (category, name), most frequent first -
  //! covering Instant and Begin events (so Begin/End scope pairs count
  //! once, not twice). Meant as an at-a-glance companion to writeJson(),
  //! not a replacement for actually looking at the timeline in Perfetto.
  void printSummary(std::FILE* out_ = stdout) const
  {
    std::vector<Row> rows = gatherSortedRows();

    std::vector<std::pair<std::string, std::size_t>> counts;
    for (const auto& row : rows)
    {
      const auto& e = *row.event;
      if (e.phase != TracePhase::Instant && e.phase != TracePhase::Begin)
      {
        continue;
      }
      std::string key =
        std::string(e.category ? e.category : "") + " / " + (e.name ? e.name : "");
      auto it = std::find_if(counts.begin(), counts.end(),
        [&key](const std::pair<std::string, std::size_t>& kv_) { return kv_.first == key; });
      if (it == counts.end())
      {
        counts.emplace_back(key, 1);
      }
      else
      {
        it->second++;
      }
    }

    std::sort(counts.begin(), counts.end(),
      [](const std::pair<std::string, std::size_t>& a_, const std::pair<std::string, std::size_t>& b_)
      { return a_.second > b_.second; });

    for (const auto& kv : counts)
    {
      std::fprintf(out_, "%8llu  %s\n", static_cast<unsigned long long>(kv.second), kv.first.c_str());
    }
  }

private:
  struct ThreadBuffer
  {
    std::string name;
    std::array<TraceEvent, kEventsPerThread> events;
    std::atomic<std::size_t> writeIndex{0};
  };

  struct Row
  {
    const TraceEvent* event;
    std::size_t tid;
  };

  std::vector<Row> gatherSortedRows() const
  {
    std::vector<const ThreadBuffer*> buffers;
    {
      std::lock_guard<std::mutex> lock(m_registryMtx);
      for (const auto& buf : m_threadBuffers)
      {
        buffers.push_back(buf.get());
      }
    }

    std::vector<Row> rows;
    for (std::size_t tid = 0; tid < buffers.size(); tid++)
    {
      const auto* buf = buffers[tid];
      std::size_t written = buf->writeIndex.load();
      std::size_t count = (written < kEventsPerThread) ? written : kEventsPerThread;
      for (std::size_t i = 0; i < count; i++)
      {
        rows.push_back({&buf->events[i], tid});
      }
    }

    // Physical ring-buffer slot order isn't chronological once a buffer has
    // wrapped - sort globally by timestamp so the output (and any B/E
    // pairing within it) is well-formed regardless.
    std::sort(rows.begin(), rows.end(),
      [](const Row& a_, const Row& b_) { return a_.event->timestampUs < b_.event->timestampUs; });

    return rows;
  }

  TraceRecorder() : m_startTime(std::chrono::steady_clock::now())
  {
  }

  ThreadBuffer& threadBuffer()
  {
    thread_local ThreadBuffer* cached = nullptr;
    if (!cached)
    {
      std::lock_guard<std::mutex> lock(m_registryMtx);
      m_threadBuffers.push_back(std::unique_ptr<ThreadBuffer>(new ThreadBuffer()));
      cached = m_threadBuffers.back().get();
    }
    return *cached;
  }

  void record(const char* category_, const char* name_, TracePhase phase_, int32_t value_)
  {
    auto& buf = threadBuffer();
    std::size_t idx = buf.writeIndex.fetch_add(1, std::memory_order_relaxed) % kEventsPerThread;
    auto& e = buf.events[idx];
    e.name = name_;
    e.category = category_;
    e.phase = phase_;
    e.counterValue = value_;
    e.timestampUs = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - m_startTime)
                      .count();
  }

  static void writeEscaped(std::FILE* file_, const char* s_)
  {
    for (const char* p = s_; *p; p++)
    {
      if (*p == '"' || *p == '\\')
      {
        std::fputc('\\', file_);
      }
      std::fputc(*p, file_);
    }
  }

  static void writeEvent(std::FILE* file_, const TraceEvent& e_, std::size_t tid_, bool first_)
  {
    if (!first_)
    {
      std::fputs(",\n", file_);
    }
    std::fputs("{\"name\":\"", file_);
    writeEscaped(file_, e_.name ? e_.name : "");
    std::fputs("\",\"cat\":\"", file_);
    writeEscaped(file_, e_.category ? e_.category : "");
    std::fprintf(file_, "\",\"ph\":\"%c\",\"ts\":%lld,\"pid\":1,\"tid\":%llu",
      static_cast<char>(e_.phase), static_cast<long long>(e_.timestampUs),
      static_cast<unsigned long long>(tid_));
    if (e_.phase == TracePhase::Counter)
    {
      std::fprintf(file_, ",\"args\":{\"value\":%d}", e_.counterValue);
    }
    std::fputs("}", file_);
  }

  static void writeThreadNameMeta(
    std::FILE* file_, std::size_t tid_, const std::string& name_, bool first_)
  {
    if (!first_)
    {
      std::fputs(",\n", file_);
    }
    std::fprintf(file_, "{\"name\":\"thread_name\",\"ph\":\"M\",\"pid\":1,\"tid\":%llu,\"args\":{\"name\":\"",
      static_cast<unsigned long long>(tid_));
    writeEscaped(file_, name_.empty() ? "unnamed" : name_.c_str());
    std::fputs("\"}}", file_);
  }

  mutable std::mutex m_registryMtx;
  std::vector<std::unique_ptr<ThreadBuffer>> m_threadBuffers;
  std::chrono::steady_clock::time_point m_startTime;
};

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

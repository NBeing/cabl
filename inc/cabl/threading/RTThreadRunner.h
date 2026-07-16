/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "cabl/trace/Trace.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

//! Runs a caller-supplied function repeatedly on a dedicated thread, paced
//! to a fixed period - portable (std::thread/std::atomic/std::chrono only,
//! no OS-specific timers), and RAII: the destructor stops and joins
//! automatically, so a runner can never outlive its own thread.
//!
//! Meant for decoupling latency-sensitive periodic work (e.g. MIDI output)
//! from whatever else a device's main tick loop happens to be blocked on.
class RTThreadRunner
{
public:
  RTThreadRunner() = default;

  ~RTThreadRunner()
  {
    stop();
  }

  RTThreadRunner(const RTThreadRunner&) = delete;
  RTThreadRunner& operator=(const RTThreadRunner&) = delete;

  //! Starts calling tickFn_ every period_ until stop() is called. No-op if
  //! already running. name_ labels this thread's track in trace output
  //! (see cabl/trace/Trace.h) - purely diagnostic, no effect when tracing
  //! is compiled out.
  void start(
    std::chrono::microseconds period_, std::function<void()> tickFn_, const char* name_ = "RT Thread")
  {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
    {
      return; // Already running
    }

    // Stored on the object rather than an init-capture (C++14) so this
    // stays buildable on stricter C++11-only toolchains too.
    m_tickFn = std::move(tickFn_);

    m_thread = std::thread(
      [this, period_, name_]()
      {
        CABL_TRACE_THREAD_NAME(name_);
        auto nextTick = std::chrono::steady_clock::now();
        while (m_running.load(std::memory_order_relaxed))
        {
          {
            CABL_TRACE_SCOPE("thread", name_);
            m_tickFn();
          }
          nextTick += period_;
          std::this_thread::sleep_until(nextTick);
        }
      });
  }

  //! Stops and joins. Safe to call when not running, or more than once.
  void stop()
  {
    m_running = false;
    if (m_thread.joinable())
    {
      m_thread.join();
    }
  }

  bool running() const
  {
    return m_running.load(std::memory_order_relaxed);
  }

private:
  std::atomic<bool> m_running{false};
  std::thread m_thread;
  std::function<void()> m_tickFn;
};

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "../core/IStepClock.h"

namespace sl
{
namespace seq
{

// std::thread + sleep_for backed IStepClock, for desktop targets - the same
// technique Euklid::play() uses inline, factored out so it's written once
// instead of copy-pasted per example. This is deliberately the *only* file
// in seqlib that touches std::thread: a future embedded backend (e.g. a
// FreeRTOS task, or a hardware-timer ISR feeding a queue) is a sibling
// IStepClock implementation, not a change to this one or to anything in
// core/.
class ThreadedStepClock : public IStepClock
{
public:
  explicit ThreadedStepClock(std::size_t stepCount_ = 16) : m_stepCount(stepCount_)
  {
  }

  ~ThreadedStepClock() override
  {
    stop();
  }

  void start(double bpm_) override
  {
    if (m_running.load())
    {
      return;
    }
    m_bpm = bpm_;
    m_running = true;
    m_thread = std::thread(&ThreadedStepClock::run, this);
  }

  void stop() override
  {
    if (!m_running.exchange(false))
    {
      return;
    }
    if (m_thread.joinable())
    {
      m_thread.join();
    }
  }

  void setBpm(double bpm_) override
  {
    m_bpm = bpm_;
  }

  void setStepCount(std::size_t stepCount_) override
  {
    m_stepCount = (stepCount_ == 0) ? 1 : stepCount_;
  }

  bool isRunning() const override
  {
    return m_running.load();
  }

private:
  void run()
  {
    std::size_t step = 0;
    while (m_running.load())
    {
      fireStep(step);

      const double bpm = m_bpm.load();
      const double stepMs = (60000.0 / bpm) / 4.0; // 16th-note steps
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(stepMs)));

      const std::size_t count = m_stepCount.load();
      step = (step + 1) % count;
    }
  }

  std::atomic<bool> m_running{false};
  std::atomic<double> m_bpm{120.0};
  std::atomic<std::size_t> m_stepCount;
  std::thread m_thread;
};

} // namespace seq
} // namespace sl

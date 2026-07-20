/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstddef>
#include <functional>

namespace sl
{
namespace seq
{

// Abstract step clock: something that calls back once per sequencer step at
// a tempo-derived rate. Deliberately has no implementation here and no
// threading primitives in its interface - only a concrete backend (e.g.
// desktop/ThreadedStepClock.h, or a future FreeRTOS-task-based backend)
// knows how ticks actually get scheduled on a given platform. Pattern/
// StepLane never depend on any of that.
class IStepClock
{
public:
  virtual ~IStepClock() = default;

  virtual void start(double bpm_) = 0;
  virtual void stop() = 0;
  virtual void setBpm(double bpm_) = 0;
  virtual void setStepCount(std::size_t stepCount_) = 0;
  virtual bool isRunning() const = 0;

  // The callback may be invoked from whatever internal timing context the
  // backend uses (e.g. a dedicated thread) - implementations must treat it
  // as running off the main/UI thread and keep it non-blocking.
  void setOnStep(std::function<void(std::size_t)> onStep_)
  {
    m_onStep = std::move(onStep_);
  }

protected:
  void fireStep(std::size_t stepIndex_)
  {
    if (m_onStep)
    {
      m_onStep(stepIndex_);
    }
  }

private:
  std::function<void(std::size_t)> m_onStep;
};

} // namespace seq
} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "StepLane.h"
#include "SynthProfile.h"

namespace sl
{
namespace seq
{

// 16 pads are the physical window, not the data limit: kMaxPatternSteps
// caps a pattern at 4 pages of 16 (paged via PageLeft/PageRight in the app
// layer). Raise this if a longer pattern turns out to be musically useful -
// it's a template bound, not a hardware constant.
constexpr std::size_t kMaxPatternSteps = 64;

// Per-step note override for the trig lane. gateSteps is reserved for a
// future multi-step-sustain feature; v1 players may ignore it and treat
// every trig as a short pulse.
struct NoteEvent
{
  uint8_t note{60};
  uint8_t velocity{100};
  uint8_t gateSteps{1};
};

using TrigLane = StepLane<NoteEvent, kMaxPatternSteps>;
using CcLane = StepLane<uint8_t, kMaxPatternSteps>;

// One CC lock lane: which synth parameter it targets, its current
// live/base value (what an unlocked step effectively plays, and what
// turning the lane's encoder edits), and the per-step locks themselves.
struct CcLockLane
{
  ParamDef param;
  uint8_t liveValue{64};
  CcLane lane;
};

// One sequencer pattern: a single trig lane plus as many CC lock lanes as
// the loaded SynthProfile defines. Lane count is therefore
// profile-sized, not hardware-sized - only 8 lanes are "live" on the
// physical encoders at once (an app-layer paging concern), the data model
// holds all of them.
class Pattern
{
public:
  explicit Pattern(std::size_t stepCount_ = 16) : m_stepCount(clampSteps(stepCount_))
  {
  }

  std::size_t stepCount() const
  {
    return m_stepCount;
  }

  void setStepCount(std::size_t stepCount_)
  {
    m_stepCount = clampSteps(stepCount_);
  }

  TrigLane& trigLane()
  {
    return m_trigLane;
  }

  const TrigLane& trigLane() const
  {
    return m_trigLane;
  }

  // Rebuilds the CC lock lanes from a synth profile. Existing locks are
  // discarded - call this once at startup / on profile change, not per
  // frame.
  void setLanes(const std::vector<ParamDef>& params_)
  {
    m_ccLanes.clear();
    m_ccLanes.reserve(params_.size());
    for (const ParamDef& param : params_)
    {
      CcLockLane lockLane;
      lockLane.param = param;
      m_ccLanes.push_back(lockLane);
    }
  }

  std::size_t laneCount() const
  {
    return m_ccLanes.size();
  }

  CcLockLane& lane(std::size_t index_)
  {
    return m_ccLanes[index_];
  }

  const CcLockLane& lane(std::size_t index_) const
  {
    return m_ccLanes[index_];
  }

  static constexpr std::size_t maxSteps()
  {
    return kMaxPatternSteps;
  }

private:
  static std::size_t clampSteps(std::size_t stepCount_)
  {
    if (stepCount_ < 1)
    {
      return 1;
    }
    if (stepCount_ > kMaxPatternSteps)
    {
      return kMaxPatternSteps;
    }
    return stepCount_;
  }

  std::size_t m_stepCount;
  TrigLane m_trigLane;
  std::vector<CcLockLane> m_ccLanes;
};

} // namespace seq
} // namespace sl

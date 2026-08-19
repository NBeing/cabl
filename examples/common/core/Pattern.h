/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <algorithm>
#include <array>
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

// Similarly, the physical encoders only ever show 8 lanes at a time (an
// app-layer paging concern) - this just bounds how many CC lanes a loaded
// synth profile may define. Generous relative to the 32-param Hydrasynth
// profile shipped today.
constexpr std::size_t kMaxLanes = 128;

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
// turning the lane's encoder edits - UI-thread-only, never read by the step
// clock, so it stays a plain uint8_t), and the per-step locks themselves
// (StepLane, atomic - read every tick by the step clock).
struct CcLockLane
{
  ParamDef param;
  uint8_t liveValue{64};
  CcLane lane;
};

// One sequencer pattern: a single trig lane plus as many CC lock lanes as
// the loaded SynthProfile defines (up to kMaxLanes). Lane count is
// therefore profile-sized, not hardware-sized - only 8 lanes are "live" on
// the physical encoders at once (an app-layer paging concern), the data
// model holds all of them.
//
// CC lanes live in a fixed std::array rather than a std::vector: StepLane
// holds std::atomic fields (see StepLane.h) to stay safe under concurrent
// step-clock/UI-thread access, which makes it neither copyable nor
// movable - and a std::vector's growth path requires its element type to
// be at least move-constructible, even for reserve()+emplace_back() usage.
// A fixed array sidesteps that entirely and matches StepLane's own
// fixed-capacity philosophy (kMaxPatternSteps) - no heap growth either,
// which is friendlier to the eventual microcontroller target.
class Pattern
{
public:
  explicit Pattern(std::size_t stepCount_ = 16) : m_stepCount(clampSteps(stepCount_))
  {
  }

  Pattern(const Pattern&) = delete;
  Pattern& operator=(const Pattern&) = delete;

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
  // frame, and never while a step clock might be reading this Pattern
  // concurrently (it touches more than one lane's identity at once, which
  // per-field atomics don't cover - see PlockSeq's stopped-only guard on
  // bulk pattern edits).
  void setLanes(const std::vector<ParamDef>& params_)
  {
    m_laneCount = std::min(params_.size(), kMaxLanes);
    for (std::size_t i = 0; i < m_laneCount; i++)
    {
      m_ccLanes[i].param = params_[i];
    }
  }

  std::size_t laneCount() const
  {
    return m_laneCount;
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

  static constexpr std::size_t maxLanes()
  {
    return kMaxLanes;
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
  std::array<CcLockLane, kMaxLanes> m_ccLanes{};
  std::size_t m_laneCount{0};
};

} // namespace seq
} // namespace sl

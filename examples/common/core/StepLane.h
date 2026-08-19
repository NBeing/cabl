/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace sl
{
namespace seq
{

// A fixed-capacity sequence of N steps, each carrying an on/off flag plus an
// optional locked value of type T. Deliberately generic and otherwise
// dependency-free (no I/O, no threading primitives beyond std::atomic) so it
// can back both a note trig lane (StepLane<NoteEvent, N>) and a per-parameter
// CC lock lane (StepLane<uint8_t, N>) - see Pattern.h. How a lane's "active"
// bits get populated (hand-toggled, Euclidean-generated, loaded from disk,
// ...) is deliberately not this class's concern.
//
// Fields are std::atomic: in a real sequencer app, one thread (the step
// clock) reads a step's active/locked/value every tick while a different
// thread (button/encoder handlers) writes them live during playback (that's
// the whole point of a Digitakt-style hold-pad-and-turn-an-encoder p-lock
// gesture - it has to work while running). Plain fields there would be a
// genuine data race, not a theoretical one. Relaxed ordering is enough here:
// each field is independently meaningful and there's no cross-field
// invariant that needs to stay consistent within a single step (worst case,
// a reader sees an old value for one field and a new one for another, for
// at most one clock tick - harmless for a sequencer's musical output).
// T must be trivially copyable for std::atomic<T> to be well-formed - true
// for both current payloads (uint8_t, NoteEvent).
template <typename T, std::size_t N>
class StepLane
{
public:
  struct Step
  {
    std::atomic<bool> active{false};
    std::atomic<bool> locked{false};
    std::atomic<T> value{};

    Step() = default;
  };

  static constexpr std::size_t kCapacity = N;

  StepLane() = default;

  // Atomics make Step (and therefore StepLane) neither copyable nor
  // movable - by design, so nothing can accidentally bypass the atomic
  // accessors below by copying a Step out and back in.
  StepLane(const StepLane&) = delete;
  StepLane& operator=(const StepLane&) = delete;

  bool isActive(std::size_t step_) const
  {
    return valid(step_) && m_steps[step_].active.load(std::memory_order_relaxed);
  }

  void setActive(std::size_t step_, bool active_)
  {
    if (valid(step_))
    {
      m_steps[step_].active.store(active_, std::memory_order_relaxed);
    }
  }

  void toggleActive(std::size_t step_)
  {
    if (valid(step_))
    {
      auto& active = m_steps[step_].active;
      active.store(!active.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
  }

  bool isLocked(std::size_t step_) const
  {
    return valid(step_) && m_steps[step_].locked.load(std::memory_order_relaxed);
  }

  T lockedValue(std::size_t step_) const
  {
    return valid(step_) ? m_steps[step_].value.load(std::memory_order_relaxed) : T{};
  }

  void setLock(std::size_t step_, const T& value_)
  {
    if (valid(step_))
    {
      m_steps[step_].value.store(value_, std::memory_order_relaxed);
      m_steps[step_].locked.store(true, std::memory_order_relaxed);
    }
  }

  void clearLock(std::size_t step_)
  {
    if (valid(step_))
    {
      m_steps[step_].locked.store(false, std::memory_order_relaxed);
    }
  }

  static constexpr std::size_t capacity()
  {
    return N;
  }

private:
  bool valid(std::size_t step_) const
  {
    return step_ < N;
  }

  std::array<Step, N> m_steps{};
};

} // namespace seq
} // namespace sl

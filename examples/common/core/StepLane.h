/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <array>
#include <cstddef>

namespace sl
{
namespace seq
{

// A fixed-capacity sequence of N steps, each carrying an on/off flag plus an
// optional locked value of type T. Deliberately generic and dependency-free
// (no I/O, no threading) so it can back both a note trig lane
// (StepLane<NoteEvent, N>) and a per-parameter CC lock lane
// (StepLane<uint8_t, N>) - see Pattern.h. How a lane's "active" bits get
// populated (hand-toggled, Euclidean-generated, loaded from disk, ...) is
// deliberately not this class's concern.
template <typename T, std::size_t N>
class StepLane
{
public:
  struct Step
  {
    bool active{false};
    bool locked{false};
    T value{};
  };

  static constexpr std::size_t kCapacity = N;

  bool isActive(std::size_t step_) const
  {
    return valid(step_) && m_steps[step_].active;
  }

  void setActive(std::size_t step_, bool active_)
  {
    if (valid(step_))
    {
      m_steps[step_].active = active_;
    }
  }

  void toggleActive(std::size_t step_)
  {
    if (valid(step_))
    {
      m_steps[step_].active = !m_steps[step_].active;
    }
  }

  bool isLocked(std::size_t step_) const
  {
    return valid(step_) && m_steps[step_].locked;
  }

  const T& lockedValue(std::size_t step_) const
  {
    static const T kEmpty{};
    return valid(step_) ? m_steps[step_].value : kEmpty;
  }

  void setLock(std::size_t step_, const T& value_)
  {
    if (valid(step_))
    {
      m_steps[step_].locked = true;
      m_steps[step_].value = value_;
    }
  }

  void clearLock(std::size_t step_)
  {
    if (valid(step_))
    {
      m_steps[step_].locked = false;
    }
  }

  const Step& step(std::size_t step_) const
  {
    return m_steps[step_];
  }

  Step& step(std::size_t step_)
  {
    return m_steps[step_];
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

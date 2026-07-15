/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/util/Color.h>

#include <algorithm>
#include <cstdint>

// LED facts about the Maschine MK1 that aren't obvious from cabl's generic
// Device API - confirmed empirically against hardware, not just read from
// source. Callers work in brightness ratios [0.0, 1.0]; the 7-bit byte lives
// only in here.
namespace sl
{
namespace mk1led
{

// The MK1's button and pad LEDs take a 7-bit brightness value. 128 or higher
// doesn't clip to max - it wraps/misbehaves, even though cabl's Color class
// itself accepts a full 0-255 byte.
constexpr uint8_t kMaxBrightness = 127;

// The pad grid is a 4x4 array. Raw index 0 = top-left, 15 = bottom-right, in
// row-major reading order - confirmed against physical hardware. No remap is
// needed despite MaschineMK1::led(unsigned) using a different internal order
// for its own LED-buffer bookkeeping.
constexpr unsigned kNumPads = 16;

inline cabl::Color brightness(double ratio_)
{
  ratio_ = std::max(0.0, std::min(1.0, ratio_));
  return cabl::Color(static_cast<uint8_t>(ratio_ * kMaxBrightness));
}

inline cabl::Color off()
{
  return cabl::Color(static_cast<uint8_t>(0));
}

inline cabl::Color full()
{
  return brightness(1.0);
}

} // namespace mk1led
} // namespace sl

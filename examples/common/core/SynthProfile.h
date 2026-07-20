/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sl
{
namespace seq
{

// A single named MIDI CC parameter, e.g. {74, "F1 Cutoff"} for a Hydrasynth.
// Generalizes what used to be ProgramChange::ParamDef so it can be shared
// across examples instead of redeclared per app.
struct ParamDef
{
  uint8_t cc{0};
  std::string name;

  ParamDef() = default;
  ParamDef(uint8_t cc_, std::string name_) : cc(cc_), name(std::move(name_))
  {
  }
};

// The full named CC map for one synth (e.g. everything in a
// hydrasynth.ccmap file). Deliberately just data - loading it from disk is
// a desktop/-only concern (see SynthProfileLoader.h) so a different backend
// (e.g. reading from flash on a microcontroller) can fill the same struct.
struct SynthProfile
{
  std::string name;
  std::vector<ParamDef> params;
};

} // namespace seq
} // namespace sl

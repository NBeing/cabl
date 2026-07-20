/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstdint>
#include <fstream>
#include <regex>
#include <string>

#include "../core/SynthProfile.h"

namespace sl
{
namespace seq
{

// Desktop-only loader for a ".ccmap" file: plain text, one "<cc> <name>"
// pair per line, blank lines and '#' comments ignored, e.g.:
//
//   # Hydrasynth
//   74    F1 Cutoff
//   71    F1 Resonance
//
// Same line-based/tolerant-parsing style as PatchNamesLoader.h. Fills a
// plain SynthProfile struct so the *format* is what's swappable per
// platform - a future embedded port supplies an equivalent loader backed
// by flash/LittleFS filling the same struct, without SynthProfile itself
// changing.
class SynthProfileLoader
{
public:
  static bool loadFromFile(const std::string& filepath_, SynthProfile& outProfile_)
  {
    std::ifstream file(filepath_);
    if (!file.is_open())
    {
      return false;
    }

    outProfile_.params.clear();
    outProfile_.name = profileNameFromPath(filepath_);

    static const std::regex kCcLine(R"(^\s*(\d{1,3})\s+(.+?)\s*$)");
    std::string line;
    while (std::getline(file, line))
    {
      if (line.empty() || line[0] == '#')
      {
        continue;
      }

      std::smatch match;
      if (std::regex_match(line, match, kCcLine))
      {
        const int cc = std::stoi(match[1]);
        if (cc >= 0 && cc <= 127)
        {
          outProfile_.params.emplace_back(static_cast<uint8_t>(cc), match[2].str());
        }
      }
    }

    return true;
  }

private:
  static std::string profileNameFromPath(const std::string& filepath_)
  {
    std::size_t start = filepath_.find_last_of("/\\");
    start = (start == std::string::npos) ? 0 : start + 1;
    std::size_t end = filepath_.find_last_of('.');
    if (end == std::string::npos || end < start)
    {
      end = filepath_.size();
    }
    return filepath_.substr(start, end - start);
  }
};

} // namespace seq
} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <regex>

namespace sl
{

// Runtime loader for Hydrasynth patch names from text file
class PatchNamesLoader
{
public:
  static constexpr unsigned kNumBanks = 8;      // A-H
  static constexpr unsigned kProgramsPerBank = 128;
  static constexpr unsigned kMaxNameLength = 64;

  using PatchDatabase = std::array<std::array<std::string, kProgramsPerBank>, kNumBanks>;

  // Load patch names from file
  static bool loadFromFile(const std::string& filepath, PatchDatabase& outPatches)
  {
    // Initialize with empty strings
    for (auto& bank : outPatches)
    {
      for (auto& patch : bank)
      {
        patch.clear();
      }
    }

    std::ifstream file(filepath);
    if (!file.is_open())
    {
      return false;
    }

    std::string line;
    std::regex patchLine(R"(([A-H])\s+(\d{3})\s*[\t\s]+(.+))");
    std::smatch match;

    while (std::getline(file, line))
    {
      if (line.empty())
      {
        continue;
      }

      if (std::regex_search(line, match, patchLine))
      {
        // Extract bank letter (A-H)
        std::string bankLetter = match[1];
        unsigned bankIdx = bankLetter[0] - 'A';

        // Extract program number (1-128, convert to 0-127)
        unsigned programNum = std::stoul(match[2]);
        unsigned programIdx = programNum - 1;

        // Extract patch name
        std::string patchName = match[3];

        // Trim trailing whitespace
        patchName.erase(patchName.find_last_not_of(" \t\r\n") + 1);

        // Store in database
        if (bankIdx < kNumBanks && programIdx < kProgramsPerBank)
        {
          outPatches[bankIdx][programIdx] = patchName;
        }
      }
    }

    return true;
  }

  // Get patch name (const char* for compatibility)
  static const char* getPatchName(const PatchDatabase& patches, uint8_t bank, uint8_t program)
  {
    if (bank >= kNumBanks || program >= kProgramsPerBank)
    {
      return "";
    }

    const std::string& name = patches[bank][program];
    return name.empty() ? "" : name.c_str();
  }

  // Get patch name as std::string
  static const std::string& getPatchNameString(const PatchDatabase& patches, uint8_t bank, uint8_t program)
  {
    static const std::string empty;
    if (bank >= kNumBanks || program >= kProgramsPerBank)
    {
      return empty;
    }
    return patches[bank][program];
  }
};

} // namespace sl

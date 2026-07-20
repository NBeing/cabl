/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "PlockSeq.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "desktop/SynthProfileLoader.h"

//--------------------------------------------------------------------------------------------------

void printUsage(const char* argv0_)
{
  std::cout
    << "Usage: " << argv0_
    << " --profile FILE [--channel N] [--pattern FILE] [--patches FILE] [--bpm N]\n"
    << "\n"
    << "  --profile FILE   Path to a .ccmap synth CC-name file (required, e.g.\n"
    << "                   cabl/examples/common/profiles/hydrasynth.ccmap)\n"
    << "  --channel N      MIDI channel to send on, 1-16 (default: 3)\n"
    << "  --pattern FILE   Pattern save/load path (default: patterns/pattern.plock)\n"
    << "  --patches FILE   Optional patch-name file for display (e.g. patchlist.txt)\n"
    << "  --bpm N          Starting tempo, 40-240 (default: 120)\n";
}

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  uint8_t channel = 3;
  std::string profilePath;
  std::string patternPath = "patterns/pattern.plock";
  std::string patchesPath;
  double bpm = 120.0;

  for (int i = 1; i < argc; i++)
  {
    if (std::strcmp(argv[i], "--channel") == 0 && i + 1 < argc)
    {
      int value = std::atoi(argv[++i]);
      if (value < 1 || value > 16)
      {
        std::cout << "--channel must be 1-16" << std::endl;
        return 1;
      }
      channel = static_cast<uint8_t>(value);
    }
    else if (std::strcmp(argv[i], "--profile") == 0 && i + 1 < argc)
    {
      profilePath = argv[++i];
    }
    else if (std::strcmp(argv[i], "--pattern") == 0 && i + 1 < argc)
    {
      patternPath = argv[++i];
    }
    else if (std::strcmp(argv[i], "--patches") == 0 && i + 1 < argc)
    {
      patchesPath = argv[++i];
    }
    else if (std::strcmp(argv[i], "--bpm") == 0 && i + 1 < argc)
    {
      int value = std::atoi(argv[++i]);
      if (value < 40 || value > 240)
      {
        std::cout << "--bpm must be 40-240" << std::endl;
        return 1;
      }
      bpm = static_cast<double>(value);
    }
    else
    {
      printUsage(argv[0]);
      return (std::strcmp(argv[i], "--help") == 0) ? 0 : 1;
    }
  }

  if (profilePath.empty())
  {
    std::cout << "--profile is required (e.g. cabl/examples/common/profiles/hydrasynth.ccmap)"
              << std::endl;
    printUsage(argv[0]);
    return 1;
  }

  sl::seq::SynthProfile profile;
  if (!sl::seq::SynthProfileLoader::loadFromFile(profilePath, profile))
  {
    std::cout << "Could not load synth profile from " << profilePath << std::endl;
    return 1;
  }

  sl::PlockSeq plockSeq(channel, profile, patternPath, patchesPath, bpm);

  while (std::cin.get() != 'q')
  {
    std::this_thread::yield();
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

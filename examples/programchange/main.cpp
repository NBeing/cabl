/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "ProgramChange.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

//--------------------------------------------------------------------------------------------------

void printUsage(const char* argv0_)
{
  std::cout << "Usage: " << argv0_ << " [--channel N] [--patches FILE] [--mode program|parameter]\n"
            << "\n"
            << "  --channel N      MIDI channel to send Program Change on, 1-16 (default: 3)\n"
            << "  --patches FILE   Path to Hydrasynth patch names file (optional)\n"
            << "  --mode MODE      program or parameter (default: program)\n";
}

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  uint8_t channel = 3;
  std::string patchesFile;
  sl::ProgramChange::Mode mode = sl::ProgramChange::Mode::Program;

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
    else if (std::strcmp(argv[i], "--patches") == 0 && i + 1 < argc)
    {
      patchesFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "--mode") == 0 && i + 1 < argc)
    {
      const char* modeArg = argv[++i];
      if (std::strcmp(modeArg, "program") == 0)
      {
        mode = sl::ProgramChange::Mode::Program;
      }
      else if (std::strcmp(modeArg, "parameter") == 0)
      {
        mode = sl::ProgramChange::Mode::Parameter;
      }
      else
      {
        std::cout << "--mode must be 'program' or 'parameter'" << std::endl;
        return 1;
      }
    }
    else
    {
      printUsage(argv[0]);
      return (std::strcmp(argv[i], "--help") == 0) ? 0 : 1;
    }
  }

  sl::ProgramChange programChange(channel, patchesFile, mode);

  while (std::cin.get() != 'q')
  {
    std::this_thread::yield();
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

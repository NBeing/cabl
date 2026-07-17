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

namespace
{

void printUsage(const char* argv0_)
{
  std::cout << "Usage: " << argv0_ << " [--channel N]\n"
            << "\n"
            << "  --channel N   MIDI channel to send Program Change on, 1-16 (default: 3)\n";
}

} // namespace

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  uint8_t channel = 3;

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
    else
    {
      printUsage(argv[0]);
      return (std::strcmp(argv[i], "--help") == 0) ? 0 : 1;
    }
  }

  sl::ProgramChange programChange(channel);

  while (std::cin.get() != 'q')
  {
    std::this_thread::yield();
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

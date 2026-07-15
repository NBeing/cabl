/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include <cstdio>
#include <thread>
#include <iostream>
#include <chrono>

#include "KnobDisplay.h"

using namespace sl;
using namespace sl::cabl;

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  std::cout << "=== CABL Pad Display Example ===" << std::endl;
  std::cout << "Hit pads 1-4 to see velocity bars on display 1" << std::endl;
  std::cout << "Hit pads 5-8 to see velocity bars on display 2" << std::endl;
  std::cout << "Bars will fade over time after each hit" << std::endl;
  std::cout << "Type 'q' and hit ENTER to quit." << std::endl;
  std::cout << std::endl;

  KnobDisplay knobDisplay;

  while (std::cin.get() != 'q')
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}

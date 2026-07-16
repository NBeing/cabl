/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include <cabl/trace/Trace.h>

#include <iostream>
#include <thread>

#include "ScreenTest.h"

using namespace sl;
using namespace sl::cabl;

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  CABL_TRACE_THREAD_NAME("Main");

  {
    ScreenTest screenTest;

    std::cout << "Cycling through the Canvas API, ~2.5s per demo." << std::endl;
    std::cout << "Press any button to skip ahead. Type 'q' and hit ENTER to quit." << std::endl;

    while (std::cin.get() != 'q')
    {
      std::this_thread::yield();
    }
  }
  TraceRecorder::instance().writeJson("screen-test-trace.json");
  std::cout << "Wrote screen-test-trace.json - load it at ui.perfetto.dev\n\n"
            << "Event counts:" << std::endl;
  TraceRecorder::instance().printSummary();

  return 0;
}

//--------------------------------------------------------------------------------------------------

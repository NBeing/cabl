/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include <cabl/trace/Trace.h>

#include <iostream>
#include <thread>

#include "LvglMenu.h"

using namespace sl;
using namespace sl::cabl;

//--------------------------------------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  CABL_TRACE_THREAD_NAME("Main");

  {
    LvglMenu lvglMenu;

    std::cout << "Type 'q' and hit ENTER to quit." << std::endl;

    while (std::cin.get() != 'q')
    {
      std::this_thread::yield();
    }
  }
  // lvglMenu is destroyed above, but note Coordinator is a process-wide
  // singleton (Coordinator::instance()) whose background thread - and the
  // MK1's MIDI fast thread - keep running independent of any one Client's
  // lifetime, so this dump is a best-effort snapshot rather than a fully
  // quiesced one (see TraceRecorder's class comment).
  TraceRecorder::instance().writeJson("lvgl-menu-trace.json");
  std::cout << "Wrote lvgl-menu-trace.json - load it at ui.perfetto.dev" << std::endl;

  return 0;
}

//--------------------------------------------------------------------------------------------------

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "EventLoadHarness.h"

#include <cabl/trace/Trace.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{

void printUsage(const char* argv0_)
{
  std::cout << "Usage: " << argv0_ << " [options]\n"
            << "\n"
            << "Stress-tests cabl::RTSafeEventDistributor with a synthetic RT producer thread\n"
            << "and shows a live dashboard of queue depth, throughput, and drops. No MK1 or\n"
            << "any other hardware is needed - this only exercises the lock-free queue.\n"
            << "\n"
            << "Options:\n"
            << "  --rate N          RT events/sec (default: 0 = unthrottled, worst case)\n"
            << "  --duration N      run time in seconds (default: 10)\n"
            << "  --ui-interval N   simulated UI drain period in ms (default: 16, ~60fps)\n"
            << "  --ui-stall N      extra ms added per UI tick, simulates a busy UI (default: 0)\n"
            << "  --dashboard N     dashboard refresh period in ms (default: 200)\n"
            << "  --help            show this message\n"
            << "\n"
            << "Examples:\n"
            << "  " << argv0_ << "                          # baseline: fast producer, 60fps drain\n"
            << "  " << argv0_ << " --rate 1000               # a realistic bounded MIDI-ish rate\n"
            << "  " << argv0_ << " --ui-stall 100             # simulate a UI thread that's mostly busy\n"
            << "  " << argv0_ << " --rate 50000 --ui-stall 50 # force visible drops\n";
}

} // namespace

//--------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  CABL_TRACE_THREAD_NAME("Main");

  sl::LoadHarnessConfig config;

  for (int i = 1; i < argc; i++)
  {
    auto next = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : nullptr; };

    if (std::strcmp(argv[i], "--rate") == 0 && next())
    {
      config.rtEventsPerSecond = std::atof(argv[i]);
    }
    else if (std::strcmp(argv[i], "--duration") == 0 && next())
    {
      config.durationSeconds = static_cast<unsigned>(std::atoi(argv[i]));
    }
    else if (std::strcmp(argv[i], "--ui-interval") == 0 && next())
    {
      config.uiIntervalMs = static_cast<unsigned>(std::atoi(argv[i]));
    }
    else if (std::strcmp(argv[i], "--ui-stall") == 0 && next())
    {
      config.uiStallMs = static_cast<unsigned>(std::atoi(argv[i]));
    }
    else if (std::strcmp(argv[i], "--dashboard") == 0 && next())
    {
      config.dashboardIntervalMs = static_cast<unsigned>(std::atoi(argv[i]));
    }
    else
    {
      printUsage(argv[0]);
      return (std::strcmp(argv[i], "--help") == 0) ? 0 : 1;
    }
  }

  sl::EventLoadHarness harness(config);
  harness.run();

  sl::cabl::TraceRecorder::instance().writeJson("event-load-trace.json");
  std::cout << "\nWrote event-load-trace.json - load it at ui.perfetto.dev" << std::endl;

  return 0;
}

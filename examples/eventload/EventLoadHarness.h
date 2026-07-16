/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/threading/RTSafeEventDistributor.h>

#include <atomic>
#include <chrono>

namespace sl
{

//--------------------------------------------------------------------------------------------------

struct LoadHarnessConfig
{
  // 0 = fire events as fast as the CPU allows (worst case for the queue);
  // otherwise the RT thread paces itself to this rate.
  double rtEventsPerSecond{0.0};
  unsigned durationSeconds{10};
  // How often the "UI thread" (this process's main thread) drains the
  // queue - stands in for LVGL's lv_timer_handler tick rate.
  unsigned uiIntervalMs{16};
  // Extra delay added on top of uiIntervalMs before each drain, to simulate
  // a UI thread that's busy doing real work (redraws, allocation, etc).
  unsigned uiStallMs{0};
  unsigned dashboardIntervalMs{200};
};

//--------------------------------------------------------------------------------------------------

// Drives cabl::RTSafeEventDistributor with a synthetic RT producer thread
// and drains it from the calling thread, printing a live terminal
// dashboard of queue depth, throughput, drops, and RT dispatch timing.
// Exists to make the distributor's backpressure/drop behavior visible and
// tunable without needing any MK1 hardware attached.
class EventLoadHarness
{
public:
  explicit EventLoadHarness(const LoadHarnessConfig& config_);

  void run();

private:
  void rtProducerLoop();
  void printDashboard(double elapsedSeconds_, bool finalLine_);

  LoadHarnessConfig m_config;
  cabl::RTSafeEventDistributor m_distributor;
  std::atomic<bool> m_running{true};
  std::atomic<uint64_t> m_eventsSent{0};
  unsigned m_dashboardLineCount{0};
};

} // namespace sl

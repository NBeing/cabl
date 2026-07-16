/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "EventLoadHarness.h"

#include <cabl/trace/Trace.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <thread>

namespace sl
{

namespace
{

std::string bar(size_t value_, size_t max_, int width_)
{
  if (max_ == 0)
  {
    max_ = 1;
  }
  int filled = static_cast<int>((static_cast<double>(value_) / static_cast<double>(max_)) * width_);
  filled = std::max(0, std::min(filled, width_));
  return std::string(static_cast<size_t>(filled), '#') +
    std::string(static_cast<size_t>(width_ - filled), '-');
}

constexpr int kBarWidth = 30;
constexpr unsigned kDashboardLineCount = 9;

} // namespace

//--------------------------------------------------------------------------------------------------

EventLoadHarness::EventLoadHarness(const LoadHarnessConfig& config_) : m_config(config_)
{
  m_distributor.initialize();
}

//--------------------------------------------------------------------------------------------------

void EventLoadHarness::rtProducerLoop()
{
  CABL_TRACE_THREAD_NAME("RT Producer");

  using Clock = std::chrono::steady_clock;

  const bool paced = m_config.rtEventsPerSecond > 0.0;
  const auto interval = paced
    ? std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / m_config.rtEventsPerSecond))
    : Clock::duration::zero();

  auto nextSend = Clock::now();
  uint8_t value = 0;

  while (m_running.load(std::memory_order_relaxed))
  {
    m_distributor.notifyRTObservers(cabl::RTEvent::parameterChange(0, value++));
    m_eventsSent.fetch_add(1, std::memory_order_relaxed);

    if (paced)
    {
      nextSend += interval;
      std::this_thread::sleep_until(nextSend);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void EventLoadHarness::printDashboard(double elapsedSeconds_, bool finalLine_)
{
  auto stats = m_distributor.getStatistics();
  const uint64_t sent = m_eventsSent.load(std::memory_order_relaxed);
  const double dropPct = sent > 0 ? (100.0 * static_cast<double>(stats.rtEventsDropped) / sent) : 0.0;

  // Redraw in place (cursor up + clear each line) instead of scrolling -
  // stays readable over a slow serial console or SSH link, which matters
  // given this is meant to run headless on something like a Pi eventually.
  if (m_dashboardLineCount > 0)
  {
    std::printf("\x1b[%uA", m_dashboardLineCount);
  }
  m_dashboardLineCount = kDashboardLineCount;

  std::printf("\x1b[2K=== cabl RTSafeEventDistributor load harness ===\n");
  std::printf("\x1b[2Kelapsed:      %6.1fs / %us\n", elapsedSeconds_, m_config.durationSeconds);
  std::printf("\x1b[2KRT rate:      %s\n",
    m_config.rtEventsPerSecond > 0.0
      ? (std::to_string(static_cast<long>(m_config.rtEventsPerSecond)) + " events/s").c_str()
      : "max speed (unthrottled)");
  std::printf(
    "\x1b[2KUI drain:     every %ums (+%ums simulated stall)\n", m_config.uiIntervalMs, m_config.uiStallMs);
  std::printf("\x1b[2K\n");
  std::printf("\x1b[2Ksent:         %10llu\n", static_cast<unsigned long long>(sent));
  std::printf(
    "\x1b[2Kdelivered:    %10llu\n", static_cast<unsigned long long>(stats.uiEventsProcessed));
  std::printf("\x1b[2Kdropped:      %10llu  (%.2f%% of sent)\n",
    static_cast<unsigned long long>(stats.rtEventsDropped), dropPct);
  std::printf("\x1b[2Kqueue depth:  [%s] %4zu / %-4zu\n",
    bar(stats.rtToUiQueueSize, cabl::RTSafeEventDistributor::queueCapacity(), kBarWidth).c_str(),
    stats.rtToUiQueueSize, cabl::RTSafeEventDistributor::queueCapacity());
  std::printf("\x1b[2KRT dispatch:  last=%uus  max=%uus  (RT-safe budget: %uus)%s\n",
    stats.lastRTProcessingTimeUs, stats.maxRTProcessingTimeUs, cabl::RTSafeEventDistributor::kRTSafeThresholdUs,
    finalLine_ ? "  [done]" : "");

  std::fflush(stdout);
}

//--------------------------------------------------------------------------------------------------

void EventLoadHarness::run()
{
  using Clock = std::chrono::steady_clock;

  std::thread producer(&EventLoadHarness::rtProducerLoop, this);

  const auto startTime = Clock::now();
  const auto endTime = startTime + std::chrono::seconds(m_config.durationSeconds);
  auto nextDashboard = startTime;

  while (Clock::now() < endTime)
  {
    m_distributor.processUIEvents();

    if (Clock::now() >= nextDashboard)
    {
      printDashboard(
        std::chrono::duration<double>(Clock::now() - startTime).count(), /*finalLine_*/ false);
      nextDashboard += std::chrono::milliseconds(m_config.dashboardIntervalMs);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(m_config.uiIntervalMs + m_config.uiStallMs));
  }

  m_running = false;
  producer.join();

  // Fully flush so the closing dashboard's numbers add up exactly (sent ==
  // delivered + dropped) - during the run processUIEvents() is deliberately
  // bounded per call (that's the whole point), so a single call here could
  // leave a backlog sitting in the queue, uncounted either way.
  while (m_distributor.getStatistics().rtToUiQueueSize > 0)
  {
    m_distributor.processUIEvents();
  }
  printDashboard(std::chrono::duration<double>(Clock::now() - startTime).count(), /*finalLine_*/ true);
}

} // namespace sl

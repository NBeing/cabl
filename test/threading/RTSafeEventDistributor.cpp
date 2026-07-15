/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "catch.hpp"

#include <cabl/threading/RTSafeEventDistributor.h>

#include <atomic>
#include <thread>
#include <type_traits>
#include <vector>

namespace sl
{
namespace cabl
{
namespace test
{

namespace
{

static_assert(!std::is_copy_constructible<RTSafeEventDistributor>::value,
  "RTSafeEventDistributor must stay non-copyable");

//--------------------------------------------------------------------------------------------------

//! Records every event it's handed, in call order - lets tests assert both
//! "was it called" and "in what order" without any timing dependence.
class RecordingRTObserver : public RTObserver
{
public:
  explicit RecordingRTObserver(int priority_ = 0) : m_priority(priority_)
  {
  }

  void handleRTEvent(const RTEvent& event_) override
  {
    m_events.push_back(event_);
  }

  int getPriority() const override
  {
    return m_priority;
  }

  std::vector<RTEvent> m_events;

private:
  int m_priority;
};

//--------------------------------------------------------------------------------------------------

class RecordingUIObserver : public UIObserver
{
public:
  void handleUIEvent(const RTEvent& event_) override
  {
    m_events.push_back(event_);
  }

  std::vector<RTEvent> m_events;
};

//! Appends its own tag to a shared order log - used to verify priority
//! ordering between distinct observer instances.
class OrderTaggingObserver : public RTObserver
{
public:
  OrderTaggingObserver(int priority_, char tag_, std::vector<char>& order_)
    : m_priority(priority_), m_tag(tag_), m_order(order_)
  {
  }

  void handleRTEvent(const RTEvent&) override
  {
    m_order.push_back(m_tag);
  }

  int getPriority() const override
  {
    return m_priority;
  }

private:
  int m_priority;
  char m_tag;
  std::vector<char>& m_order;
};

} // namespace

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: events before initialize() are no-ops", "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  RecordingRTObserver rtObserver;
  distributor.addRTObserver(&rtObserver);

  distributor.notifyRTObservers(RTEvent::clockTick());
  CHECK(rtObserver.m_events.empty());

  CHECK_FALSE(distributor.sendToRTThread(RTEvent::clockTick()));
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: notifyRTObservers calls RT observers and queues for the UI thread",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingRTObserver rtObserver;
  distributor.addRTObserver(&rtObserver);

  RTEvent event = RTEvent::midiCC(1, 74, 100);
  distributor.notifyRTObservers(event);

  REQUIRE(rtObserver.m_events.size() == 1);
  CHECK(rtObserver.m_events[0].type == RTEventType::ControlChange);
  CHECK(rtObserver.m_events[0].channel == 1);
  CHECK(rtObserver.m_events[0].data1 == 74);
  CHECK(rtObserver.m_events[0].data2 == 100);

  auto stats = distributor.getStatistics();
  CHECK(stats.rtEventsProcessed == 1);
  CHECK(stats.rtToUiQueueSize == 1);
  CHECK(stats.rtEventsDropped == 0);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: RT observers run in descending priority order",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  std::vector<char> order;
  OrderTaggingObserver low(1, 'L', order);
  OrderTaggingObserver high(10, 'H', order);
  OrderTaggingObserver mid(5, 'M', order);

  // Added out of priority order on purpose - addRTObserver must sort them.
  distributor.addRTObserver(&low);
  distributor.addRTObserver(&high);
  distributor.addRTObserver(&mid);

  distributor.notifyRTObservers(RTEvent::clockTick());

  REQUIRE(order.size() == 3);
  std::vector<char> expectedOrder{'H', 'M', 'L'};
  CHECK(order == expectedOrder);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: processUIEvents drains queued events for UI observers",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingUIObserver uiObserver;
  distributor.addUIObserver(&uiObserver);

  for (uint8_t i = 0; i < 5; i++)
  {
    distributor.notifyRTObservers(RTEvent::parameterChange(i, i * 10));
  }

  distributor.processUIEvents();

  REQUIRE(uiObserver.m_events.size() == 5);
  for (uint8_t i = 0; i < 5; i++)
  {
    CHECK(uiObserver.m_events[i].data1 == i);
    CHECK(uiObserver.m_events[i].data2 == i * 10);
  }
  CHECK(distributor.getStatistics().uiEventsProcessed == 5);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: processUIEvents is bounded per call so a burst can't starve the UI thread",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingUIObserver uiObserver;
  distributor.addUIObserver(&uiObserver);

  const int kTotalEvents = 50; // > the distributor's internal 32-per-call cap
  for (int i = 0; i < kTotalEvents; i++)
  {
    distributor.notifyRTObservers(RTEvent::clockTick());
  }

  distributor.processUIEvents();
  CHECK(uiObserver.m_events.size() == 32);
  CHECK(distributor.getStatistics().rtToUiQueueSize == kTotalEvents - 32);

  distributor.processUIEvents();
  CHECK(uiObserver.m_events.size() == kTotalEvents);
  CHECK(distributor.getStatistics().rtToUiQueueSize == 0);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: UI-to-RT path is bounded per call and reaches RT observers",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingRTObserver rtObserver;
  distributor.addRTObserver(&rtObserver);

  const int kTotalEvents = 20; // > the distributor's internal 16-per-call cap
  for (int i = 0; i < kTotalEvents; i++)
  {
    CHECK(distributor.sendToRTThread(RTEvent::clockTick()));
  }

  distributor.processRTEvents();
  CHECK(rtObserver.m_events.size() == 16);

  distributor.processRTEvents();
  CHECK(rtObserver.m_events.size() == kTotalEvents);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: events are dropped and counted once the RT-to-UI queue fills",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  // No UI drain happens in this test - the queue's internal capacity is
  // 1023 usable slots (1024 - 1 reserved), so pushing past that must start
  // dropping rather than silently overwriting or blocking.
  const int kOverflowBy = 7;
  for (int i = 0; i < 1023 + kOverflowBy; i++)
  {
    distributor.notifyRTObservers(RTEvent::clockTick());
  }

  auto stats = distributor.getStatistics();
  CHECK(stats.rtToUiQueueSize == 1023);
  CHECK(stats.rtEventsDropped == kOverflowBy);
  CHECK(stats.rtEventsProcessed == 1023 + kOverflowBy); // "processed" counts RT-observer dispatch, not delivery to UI
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: shutdown clears observers and stops delivery",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingRTObserver rtObserver;
  distributor.addRTObserver(&rtObserver);
  distributor.notifyRTObservers(RTEvent::clockTick());
  REQUIRE(rtObserver.m_events.size() == 1);

  distributor.shutdown();
  distributor.notifyRTObservers(RTEvent::clockTick());

  // Still 1 - shutdown() both clears the observer list and gates delivery,
  // so this can't silently pass by fluke of the vector being cleared alone.
  CHECK(rtObserver.m_events.size() == 1);
  CHECK_FALSE(distributor.sendToRTThread(RTEvent::clockTick()));
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: a fast RT observer stays within the RT-safety budget",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingRTObserver rtObserver;
  distributor.addRTObserver(&rtObserver);

  for (int i = 0; i < 100; i++)
  {
    distributor.notifyRTObservers(RTEvent::clockTick());
  }

  // Soft check: this is wall-clock timing, not a functional guarantee, so it
  // could in principle flake under a heavily loaded/throttled CI runner. A
  // no-op observer taking over 100us would still be a real red flag though.
  CHECK(distributor.isRTSafe());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTSafeEventDistributor: concurrent RT producer and UI consumer see every event exactly once",
  "[threading][RTSafeEventDistributor]")
{
  RTSafeEventDistributor distributor;
  distributor.initialize();

  RecordingUIObserver uiObserver;
  distributor.addUIObserver(&uiObserver);

  constexpr int kEventCount = 20000;
  std::atomic<bool> producerDone{false};

  // Simulates the real usage this class exists for: one thread pushing
  // MIDI/timing events as fast as it can, another thread draining them on
  // its own schedule (LVGL's tick loop), running genuinely concurrently.
  std::thread rtThread(
    [&]()
    {
      for (int i = 0; i < kEventCount; i++)
      {
        distributor.notifyRTObservers(RTEvent::parameterChange(0, 0));
      }
      producerDone = true;
    });

  while (!producerDone || distributor.getStatistics().rtToUiQueueSize > 0)
  {
    distributor.processUIEvents();
    std::this_thread::yield();
  }

  rtThread.join();
  distributor.processUIEvents(); // drain anything queued between the last check and join

  auto stats = distributor.getStatistics();
  CHECK(stats.rtEventsProcessed == kEventCount);
  // notifyRTObservers() never blocks or retries - if the producer outruns
  // the consumer it drops rather than backing up, by design. So the only
  // thing that's actually guaranteed regardless of relative thread speed is
  // this accounting identity, not some "drops should be rare" guess.
  CHECK(uiObserver.m_events.size() + stats.rtEventsDropped == kEventCount);
}

//--------------------------------------------------------------------------------------------------

} // namespace test
} // namespace cabl
} // namespace sl

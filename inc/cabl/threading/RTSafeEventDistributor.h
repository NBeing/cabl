/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "cabl/threading/LockFreeQueue.h"
#include "cabl/trace/Trace.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

//! Event categories carried across the RT/UI thread boundary.
enum class RTEventType : uint8_t
{
  MidiInput = 0,
  MidiOutput = 1,
  ParameterChange = 2,
  ClockTick = 3,
  ControlChange = 4
};

//--------------------------------------------------------------------------------------------------

//! Fixed-size event, safe to pass through a lock-free queue with no
//! allocation - all payload has to fit in these four bytes plus metadata.
struct RTEvent
{
  RTEventType type{RTEventType::MidiInput};
  uint8_t channel{0};
  uint8_t data1{0};
  uint8_t data2{0};
  uint32_t timestampUs{0};
  uint32_t sourceId{0};

  RTEvent() = default;

  RTEvent(RTEventType type_, uint8_t channel_, uint8_t data1_, uint8_t data2_, uint32_t sourceId_ = 0)
    : type(type_), channel(channel_), data1(data1_), data2(data2_), sourceId(sourceId_)
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    timestampUs =
      static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(epoch).count());
  }

  static RTEvent midiCC(uint8_t channel_, uint8_t controller_, uint8_t value_)
  {
    return RTEvent(RTEventType::ControlChange, channel_, controller_, value_);
  }

  static RTEvent parameterChange(uint8_t paramId_, uint8_t value_)
  {
    return RTEvent(RTEventType::ParameterChange, 0, paramId_, value_);
  }

  static RTEvent clockTick()
  {
    return RTEvent(RTEventType::ClockTick, 0, 0, 0);
  }
};

//--------------------------------------------------------------------------------------------------

//! Observer called directly, synchronously, from the RT thread.
//!
//! handleRTEvent() MUST be RT-safe: no allocation, no blocking, no mutexes,
//! bounded execution time (aim for under 100us, under 50us on a
//! microcontroller). Anything slower belongs behind UIObserver instead.
class RTObserver
{
public:
  virtual ~RTObserver() = default;
  virtual void handleRTEvent(const RTEvent& event_) = 0;

  //! Higher priority observers are notified first.
  virtual int getPriority() const
  {
    return 0;
  }
};

//--------------------------------------------------------------------------------------------------

//! Observer called from the UI thread's drain loop - free to allocate,
//! block, or take an unbounded amount of time.
class UIObserver
{
public:
  virtual ~UIObserver() = default;
  virtual void handleUIEvent(const RTEvent& event_) = 0;
};

//--------------------------------------------------------------------------------------------------

//! Bridges an RT thread (e.g. MIDI I/O, sample-accurate timing) and a UI
//! thread (e.g. LVGL's lv_timer_handler loop) without either one blocking
//! on, or allocating for, the other.
//!
//! RT observers are called synchronously and must stay within their time
//! budget; every RT event is also queued (lock-free, non-blocking) for the
//! UI thread to pick up on its own schedule via processUIEvents(). The
//! reverse path (UI -> RT) exists for things like "user turned an encoder,
//! recompute a parameter on the RT thread" - queued by the UI thread via
//! sendToRTThread() and drained by the RT thread via processRTEvents().
class RTSafeEventDistributor
{
public:
  RTSafeEventDistributor() = default;
  ~RTSafeEventDistributor() = default;

  RTSafeEventDistributor(const RTSafeEventDistributor&) = delete;
  RTSafeEventDistributor& operator=(const RTSafeEventDistributor&) = delete;

  //! Not RT-safe - call during startup, before any thread touches the
  //! distributor.
  void initialize()
  {
    m_rtObservers.clear();
    m_uiObservers.clear();

    m_rtEventsProcessed = 0;
    m_uiEventsProcessed = 0;
    m_rtEventsDropped = 0;
    m_uiEventsDropped = 0;
    m_maxRTProcessingTimeUs = 0;
    m_lastRTProcessingTimeUs = 0;

    m_initialized = true;
  }

  void shutdown()
  {
    m_initialized = false;
    m_rtObservers.clear();
    m_uiObservers.clear();
  }

  //! Not RT-safe - call during startup only.
  bool addRTObserver(RTObserver* observer_)
  {
    if (!observer_)
    {
      return false;
    }

    m_rtObservers.push_back(observer_);
    std::sort(m_rtObservers.begin(), m_rtObservers.end(),
      [](RTObserver* a_, RTObserver* b_) { return a_->getPriority() > b_->getPriority(); });

    return true;
  }

  //! Not RT-safe - call during startup only.
  bool addUIObserver(UIObserver* observer_)
  {
    if (!observer_)
    {
      return false;
    }

    m_uiObservers.push_back(observer_);
    return true;
  }

  //! RT-safe. Call from the RT thread with a freshly-produced event: notifies
  //! every RT observer synchronously (in priority order) and enqueues the
  //! event for the UI thread.
  void notifyRTObservers(const RTEvent& event_)
  {
    if (!m_initialized)
    {
      return;
    }

    CABL_TRACE_INSTANT("rtbridge", "RT event");

    auto startTime = std::chrono::high_resolution_clock::now();

    for (auto observer : m_rtObservers)
    {
      observer->handleRTEvent(event_);
    }

    if (!m_rtToUiQueue.enqueue(event_))
    {
      m_rtEventsDropped++;
    }

    m_rtEventsProcessed++;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationUs = static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());

    m_lastRTProcessingTimeUs = durationUs;

    uint32_t currentMax = m_maxRTProcessingTimeUs.load();
    while (durationUs > currentMax
      && !m_maxRTProcessingTimeUs.compare_exchange_weak(currentMax, durationUs))
    {
      // Retry - another thread updated the max first.
    }
  }

  //! Not RT-safe. Call from the UI thread (e.g. once per lv_timer_handler
  //! tick) to drain events produced by the RT thread. Bounded per call so a
  //! burst of RT events can't starve the UI thread.
  void processUIEvents()
  {
    if (!m_initialized)
    {
      return;
    }

    CABL_TRACE_SCOPE("rtbridge", "UI drain");

    RTEvent event;
    int processedCount = 0;

    while (processedCount < kMaxUIEventsPerCall && m_rtToUiQueue.dequeue(event))
    {
      for (auto observer : m_uiObservers)
      {
        observer->handleUIEvent(event);
      }

      m_uiEventsProcessed++;
      processedCount++;
    }

    CABL_TRACE_COUNTER("rtbridge", "rtToUiQueueSize", static_cast<int32_t>(m_rtToUiQueue.size()));
  }

  //! Not RT-safe. Call from the UI thread to hand an event to the RT thread.
  bool sendToRTThread(const RTEvent& event_)
  {
    if (!m_initialized)
    {
      return false;
    }

    return m_uiToRtQueue.enqueue(event_);
  }

  //! RT-safe. Call from the RT thread to drain events sent by the UI thread.
  //! Bounded per call to keep RT processing time predictable.
  void processRTEvents()
  {
    if (!m_initialized)
    {
      return;
    }

    RTEvent event;
    int processedCount = 0;

    while (processedCount < kMaxRTEventsPerCall && m_uiToRtQueue.dequeue(event))
    {
      notifyRTObservers(event);
      processedCount++;
    }
  }

  //! Usable capacity of each direction's queue (both are sized the same).
  static constexpr std::size_t queueCapacity()
  {
    return kQueueSize - 1;
  }

  struct Statistics
  {
    uint64_t rtEventsProcessed;
    uint64_t uiEventsProcessed;
    uint64_t rtEventsDropped;
    uint64_t uiEventsDropped;
    uint32_t maxRTProcessingTimeUs;
    uint32_t lastRTProcessingTimeUs;
    std::size_t rtObserverCount;
    std::size_t uiObserverCount;
    std::size_t rtToUiQueueSize;
    std::size_t uiToRtQueueSize;
  };

  Statistics getStatistics() const
  {
    return {
      m_rtEventsProcessed.load(),
      m_uiEventsProcessed.load(),
      m_rtEventsDropped.load(),
      m_uiEventsDropped.load(),
      m_maxRTProcessingTimeUs.load(),
      m_lastRTProcessingTimeUs.load(),
      m_rtObservers.size(),
      m_uiObservers.size(),
      m_rtToUiQueue.size(),
      m_uiToRtQueue.size(),
    };
  }

  void resetStatistics()
  {
    m_rtEventsProcessed = 0;
    m_uiEventsProcessed = 0;
    m_rtEventsDropped = 0;
    m_uiEventsDropped = 0;
    m_maxRTProcessingTimeUs = 0;
    m_lastRTProcessingTimeUs = 0;
  }

  //! Best-effort check based on observed timing, not a guarantee - useful in
  //! tests/diagnostics, not as a runtime gate.
  bool isRTSafe() const
  {
    return m_maxRTProcessingTimeUs.load() < kRTSafeThresholdUs;
  }

  static constexpr uint32_t kRTSafeThresholdUs = 100;

private:
  static constexpr std::size_t kQueueSize = 1024;
  static constexpr int kMaxUIEventsPerCall = 32;
  static constexpr int kMaxRTEventsPerCall = 16;

  std::vector<RTObserver*> m_rtObservers;
  std::vector<UIObserver*> m_uiObservers;

  LockFreeQueue<RTEvent, kQueueSize> m_rtToUiQueue;
  LockFreeQueue<RTEvent, kQueueSize> m_uiToRtQueue;

  std::atomic<uint64_t> m_rtEventsProcessed{0};
  std::atomic<uint64_t> m_uiEventsProcessed{0};
  std::atomic<uint64_t> m_rtEventsDropped{0};
  std::atomic<uint64_t> m_uiEventsDropped{0};

  std::atomic<uint32_t> m_maxRTProcessingTimeUs{0};
  std::atomic<uint32_t> m_lastRTProcessingTimeUs{0};

  std::atomic<bool> m_initialized{false};
};

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

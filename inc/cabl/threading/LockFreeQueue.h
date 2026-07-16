/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstddef>
#include <array>
#include <atomic>
#include <vector>

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

//! Lock-free single-producer single-consumer ring buffer.
//!
//! Safe for exactly one producer thread calling enqueue() and one consumer
//! thread calling dequeue() concurrently - not safe for multiple producers
//! or multiple consumers. One slot is always kept empty to distinguish a
//! full buffer from an empty one, so capacity() is Capacity - 1.
template <typename T, std::size_t Capacity>
class LockFreeQueue
{
public:
  LockFreeQueue() = default;

  LockFreeQueue(const LockFreeQueue&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&) = delete;

  //! Producer side. Returns false if the queue is full.
  bool enqueue(const T& item_)
  {
    const std::size_t currentTail = m_tail.load(std::memory_order_relaxed);
    const std::size_t nextTail = (currentTail + 1) % Capacity;

    if (nextTail == m_head.load(std::memory_order_acquire))
    {
      return false; // Full
    }

    m_buffer[currentTail] = item_;
    m_tail.store(nextTail, std::memory_order_release);
    return true;
  }

  //! Consumer side. Returns false if the queue is empty.
  bool dequeue(T& item_)
  {
    const std::size_t currentHead = m_head.load(std::memory_order_relaxed);

    if (currentHead == m_tail.load(std::memory_order_acquire))
    {
      return false; // Empty
    }

    item_ = m_buffer[currentHead];
    m_head.store((currentHead + 1) % Capacity, std::memory_order_release);
    return true;
  }

  bool empty() const
  {
    return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
  }

  //! Approximate - may be stale by the time the caller reads it if the other
  //! side of the queue is concurrently active.
  std::size_t size() const
  {
    const std::size_t currentTail = m_tail.load(std::memory_order_acquire);
    const std::size_t currentHead = m_head.load(std::memory_order_acquire);

    return (currentTail >= currentHead) ? (currentTail - currentHead)
                                         : (Capacity - currentHead + currentTail);
  }

  constexpr std::size_t capacity() const
  {
    return Capacity - 1;
  }

private:
  std::array<T, Capacity> m_buffer;
  std::atomic<std::size_t> m_head{0};
  std::atomic<std::size_t> m_tail{0};
};

//--------------------------------------------------------------------------------------------------

//! Same contract as the fixed-capacity template, sized at construction time
//! instead of at compile time.
template <typename T>
class LockFreeQueue<T, 0>
{
public:
  explicit LockFreeQueue(std::size_t capacity_) : m_buffer(capacity_ + 1), m_capacity(capacity_ + 1)
  {
  }

  LockFreeQueue(const LockFreeQueue&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&) = delete;

  bool enqueue(const T& item_)
  {
    const std::size_t currentTail = m_tail.load(std::memory_order_relaxed);
    const std::size_t nextTail = (currentTail + 1) % m_capacity;

    if (nextTail == m_head.load(std::memory_order_acquire))
    {
      return false;
    }

    m_buffer[currentTail] = item_;
    m_tail.store(nextTail, std::memory_order_release);
    return true;
  }

  bool dequeue(T& item_)
  {
    const std::size_t currentHead = m_head.load(std::memory_order_relaxed);

    if (currentHead == m_tail.load(std::memory_order_acquire))
    {
      return false;
    }

    item_ = m_buffer[currentHead];
    m_head.store((currentHead + 1) % m_capacity, std::memory_order_release);
    return true;
  }

  bool empty() const
  {
    return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
  }

  std::size_t size() const
  {
    const std::size_t currentTail = m_tail.load(std::memory_order_acquire);
    const std::size_t currentHead = m_head.load(std::memory_order_acquire);

    return (currentTail >= currentHead) ? (currentTail - currentHead)
                                         : (m_capacity - currentHead + currentTail);
  }

  std::size_t capacity() const
  {
    return m_capacity - 1;
  }

private:
  std::vector<T> m_buffer;
  std::atomic<std::size_t> m_head{0};
  std::atomic<std::size_t> m_tail{0};
  std::size_t m_capacity;
};

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

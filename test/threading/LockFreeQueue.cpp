/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "catch.hpp"

#include <cabl/threading/LockFreeQueue.h>

#include <atomic>
#include <thread>

namespace sl
{
namespace cabl
{
namespace test
{

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: empty queue behaviour", "[threading][LockFreeQueue]")
{
  LockFreeQueue<int, 8> queue;

  CHECK(queue.empty());
  CHECK(queue.size() == 0);
  CHECK(queue.capacity() == 7); // one slot reserved to distinguish full/empty

  int value = -1;
  CHECK_FALSE(queue.dequeue(value));
  CHECK(value == -1); // untouched on failure
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: FIFO ordering", "[threading][LockFreeQueue]")
{
  LockFreeQueue<int, 8> queue;

  for (int i = 0; i < 5; i++)
  {
    CHECK(queue.enqueue(i));
  }
  CHECK(queue.size() == 5);
  CHECK_FALSE(queue.empty());

  for (int i = 0; i < 5; i++)
  {
    int value = -1;
    CHECK(queue.dequeue(value));
    CHECK(value == i);
  }
  CHECK(queue.empty());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: full queue rejects further enqueues", "[threading][LockFreeQueue]")
{
  LockFreeQueue<int, 4> queue; // capacity() == 3

  CHECK(queue.enqueue(1));
  CHECK(queue.enqueue(2));
  CHECK(queue.enqueue(3));
  CHECK_FALSE(queue.enqueue(4)); // full - one slot stays reserved
  CHECK(queue.size() == queue.capacity());

  int value = -1;
  CHECK(queue.dequeue(value));
  CHECK(value == 1);

  // Draining one slot makes room for exactly one more.
  CHECK(queue.enqueue(4));
  CHECK_FALSE(queue.enqueue(5));
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: survives repeated wraparound", "[threading][LockFreeQueue]")
{
  LockFreeQueue<int, 4> queue;

  // Push the ring buffer's internal index past its capacity many times over
  // and confirm ordering and size accounting stay correct throughout.
  int nextExpected = 0;
  int nextToSend = 0;
  for (int round = 0; round < 50; round++)
  {
    CHECK(queue.enqueue(nextToSend++));
    CHECK(queue.enqueue(nextToSend++));
    CHECK(queue.size() == 2);

    int value = -1;
    CHECK(queue.dequeue(value));
    CHECK(value == nextExpected++);
    CHECK(queue.dequeue(value));
    CHECK(value == nextExpected++);
    CHECK(queue.empty());
  }
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: dynamic-capacity specialization matches fixed-capacity contract",
  "[threading][LockFreeQueue]")
{
  LockFreeQueue<int, 0> queue(3);

  CHECK(queue.capacity() == 3);
  CHECK(queue.enqueue(1));
  CHECK(queue.enqueue(2));
  CHECK(queue.enqueue(3));
  CHECK_FALSE(queue.enqueue(4));

  int value = -1;
  CHECK(queue.dequeue(value));
  CHECK(value == 1);
  CHECK(queue.dequeue(value));
  CHECK(value == 2);
  CHECK(queue.dequeue(value));
  CHECK(value == 3);
  CHECK_FALSE(queue.dequeue(value));
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("LockFreeQueue: concurrent single-producer/single-consumer preserves order and count",
  "[threading][LockFreeQueue]")
{
  constexpr int kItemCount = 200000;
  LockFreeQueue<int, 1024> queue;

  std::atomic<bool> producerDone{false};
  std::atomic<int> droppedByProducer{0};

  std::thread producer(
    [&]()
    {
      for (int i = 0; i < kItemCount; i++)
      {
        // The queue is much smaller than the item count, so the producer
        // will regularly find it full - spin until the consumer drains
        // rather than counting that as a real drop.
        while (!queue.enqueue(i))
        {
          std::this_thread::yield();
        }
      }
      producerDone = true;
    });

  int expected = 0;
  int received = 0;
  while (!producerDone || !queue.empty())
  {
    int value = -1;
    if (queue.dequeue(value))
    {
      REQUIRE(value == expected); // a reordered or skipped item fails immediately
      expected++;
      received++;
    }
    else
    {
      std::this_thread::yield();
    }
  }

  producer.join();
  CHECK(received == kItemCount);
  CHECK(droppedByProducer == 0);
}

//--------------------------------------------------------------------------------------------------

} // namespace test
} // namespace cabl
} // namespace sl

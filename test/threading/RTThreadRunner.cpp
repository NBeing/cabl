/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "catch.hpp"

#include <cabl/threading/RTThreadRunner.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace sl
{
namespace cabl
{
namespace test
{

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: not running before start()", "[threading][RTThreadRunner]")
{
  RTThreadRunner runner;
  CHECK_FALSE(runner.running());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: calls tickFn repeatedly at roughly the requested cadence",
  "[threading][RTThreadRunner]")
{
  RTThreadRunner runner;
  std::atomic<int> callCount{0};

  runner.start(std::chrono::microseconds(1000), [&]() { callCount++; });
  CHECK(runner.running());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  runner.stop();

  // ~100 calls expected at a 1ms period over 100ms - wide tolerance since
  // this is real wall-clock scheduling, not a functional guarantee.
  int count = callCount.load();
  CHECK(count > 20);
  CHECK(count < 500);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: stop() is prompt, not bounded by a full period",
  "[threading][RTThreadRunner]")
{
  RTThreadRunner runner;
  runner.start(std::chrono::seconds(10), []() {});

  auto before = std::chrono::steady_clock::now();
  runner.stop();
  auto elapsed = std::chrono::steady_clock::now() - before;

  CHECK(elapsed < std::chrono::seconds(1));
  CHECK_FALSE(runner.running());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: stop() when never started is a safe no-op", "[threading][RTThreadRunner]")
{
  RTThreadRunner runner;
  runner.stop();
  runner.stop();
  CHECK_FALSE(runner.running());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: destructor stops the thread automatically", "[threading][RTThreadRunner]")
{
  std::atomic<int> callCount{0};

  {
    RTThreadRunner runner;
    runner.start(std::chrono::microseconds(500), [&]() { callCount++; });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  } // destructor must stop+join here.

  // If the destructor didn't actually stop the thread, callCount would keep
  // climbing after this point - proof the thread is gone, not just "didn't
  // crash".
  int countRightAfterDestruction = callCount.load();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  CHECK(callCount.load() == countRightAfterDestruction);
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("RTThreadRunner: a second start() while running is a no-op, not a second thread",
  "[threading][RTThreadRunner]")
{
  RTThreadRunner runner;
  std::atomic<int> firstCallbackCount{0};
  std::atomic<int> secondCallbackCount{0};

  runner.start(std::chrono::microseconds(500), [&]() { firstCallbackCount++; });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  // If this were allowed to actually start a second thread, both callbacks
  // would run concurrently and secondCallbackCount would climb too.
  runner.start(std::chrono::microseconds(500), [&]() { secondCallbackCount++; });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  runner.stop();

  CHECK(firstCallbackCount.load() > 0);
  CHECK(secondCallbackCount.load() == 0);
}

//--------------------------------------------------------------------------------------------------

} // namespace test
} // namespace cabl
} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "catch.hpp"

#include <cabl/trace/TraceRecorder.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace sl
{
namespace cabl
{
namespace test
{

namespace
{

// TraceRecorder is a process-wide singleton (like DeviceFactory elsewhere in
// this codebase) with no reset() - other TEST_CASEs in this binary leave
// their own threads/events sitting in it. So these tests only ever check
// "does my distinctive marker show up", never exact counts.
std::string readFile(const std::string& path_)
{
  std::ifstream in(path_);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

} // namespace

//--------------------------------------------------------------------------------------------------

TEST_CASE("TraceRecorder: instant/begin/end/counter events round-trip through writeJson",
  "[trace][TraceRecorder]")
{
  auto& recorder = TraceRecorder::instance();
  recorder.recordInstant("test-cat-instant", "test-name-instant");
  recorder.recordBegin("test-cat-scope", "test-name-scope");
  recorder.recordEnd("test-cat-scope", "test-name-scope");
  recorder.recordCounter("test-cat-counter", "test-name-counter", 42);

  const std::string path = "trace-recorder-test-basic.json";
  CHECK(recorder.writeJson(path));

  std::string contents = readFile(path);
  CHECK(contents.find("\"traceEvents\"") != std::string::npos);
  CHECK(contents.find("\"name\":\"test-name-instant\"") != std::string::npos);
  CHECK(contents.find("\"ph\":\"i\"") != std::string::npos);
  CHECK(contents.find("\"name\":\"test-name-scope\"") != std::string::npos);
  CHECK(contents.find("\"ph\":\"B\"") != std::string::npos);
  CHECK(contents.find("\"ph\":\"E\"") != std::string::npos);
  CHECK(contents.find("\"name\":\"test-name-counter\"") != std::string::npos);
  CHECK(contents.find("\"value\":42") != std::string::npos);

  std::remove(path.c_str());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("TraceRecorder: registerThreadName labels the calling thread's track", "[trace][TraceRecorder]")
{
  auto& recorder = TraceRecorder::instance();
  recorder.registerThreadName("test-thread-name-marker");
  recorder.recordInstant("test-cat", "test-name-for-thread-label");

  const std::string path = "trace-recorder-test-threadname.json";
  CHECK(recorder.writeJson(path));

  std::string contents = readFile(path);
  CHECK(contents.find("\"thread_name\"") != std::string::npos);
  CHECK(contents.find("test-thread-name-marker") != std::string::npos);

  std::remove(path.c_str());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("TraceRecorder: multiple threads record concurrently without crashing", "[trace][TraceRecorder]")
{
  auto& recorder = TraceRecorder::instance();

  std::vector<std::thread> threads;
  for (int i = 0; i < 4; i++)
  {
    threads.emplace_back(
      [i]()
      {
        auto& rec = TraceRecorder::instance();
        std::string threadName = "test-concurrent-thread-" + std::to_string(i);
        rec.registerThreadName(threadName.c_str());
        for (int j = 0; j < 100; j++)
        {
          rec.recordInstant("test-cat-concurrent", "test-name-concurrent");
        }
      });
  }
  for (auto& t : threads)
  {
    t.join();
  }

  const std::string path = "trace-recorder-test-concurrent.json";
  CHECK(recorder.writeJson(path));

  std::string contents = readFile(path);
  CHECK(contents.find("test-concurrent-thread-0") != std::string::npos);
  CHECK(contents.find("test-concurrent-thread-3") != std::string::npos);
  CHECK(contents.find("\"name\":\"test-name-concurrent\"") != std::string::npos);

  std::remove(path.c_str());
}

//--------------------------------------------------------------------------------------------------

TEST_CASE("TraceRecorder: ring buffer wraps on overflow without crashing", "[trace][TraceRecorder]")
{
  std::thread producer(
    []()
    {
      auto& rec = TraceRecorder::instance();
      rec.registerThreadName("test-wraparound-thread");
      // Comfortably more than kEventsPerThread, so the ring buffer wraps
      // at least once.
      for (std::size_t i = 0; i < TraceRecorder::kEventsPerThread + 500; i++)
      {
        rec.recordInstant("test-cat-wrap", "test-name-wrap");
      }
      rec.recordInstant("test-cat-wrap", "test-name-wrap-last");
    });
  producer.join();

  const std::string path = "trace-recorder-test-wraparound.json";
  CHECK(TraceRecorder::instance().writeJson(path));

  std::string contents = readFile(path);
  CHECK(contents.find("test-wraparound-thread") != std::string::npos);
  CHECK(contents.find("\"name\":\"test-name-wrap-last\"") != std::string::npos);

  std::remove(path.c_str());
}

//--------------------------------------------------------------------------------------------------

} // namespace test
} // namespace cabl
} // namespace sl

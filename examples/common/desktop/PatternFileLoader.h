/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

#include "../core/Pattern.h"

namespace sl
{
namespace seq
{

// Desktop-only flat-text save/load for a Pattern's step data (trig lane +
// every CC lock lane's locks). One file = one saved pattern. Plain text by
// deliberate choice (no JSON dependency) so a future embedded backend can
// swap the storage medium (e.g. a LittleFS file) without a new parser -
// only the std::ifstream/std::ofstream here would need to change.
//
// File shape:
//   steps=16
//   TRIG
//   0 active=1 locked=0 note=60 vel=100 gate=1
//   ...
//   LANE cc=74 name=F1 Cutoff
//   3 value=90
//   ...              (only locked steps are listed under a LANE)
//
// loadFromFile() matches lanes by CC number, not file order, so it's safe
// to load into a Pattern whose profile lanes were built in a different
// order than they were saved in.
class PatternFileLoader
{
public:
  static bool saveToFile(const std::string& filepath_, const Pattern& pattern_)
  {
    std::ofstream file(filepath_);
    if (!file.is_open())
    {
      return false;
    }

    file << "steps=" << pattern_.stepCount() << "\n";

    file << "TRIG\n";
    for (std::size_t i = 0; i < pattern_.stepCount(); i++)
    {
      const TrigLane& trigLane = pattern_.trigLane();
      const NoteEvent note = trigLane.lockedValue(i);
      file << i << " active=" << (trigLane.isActive(i) ? 1 : 0)
           << " locked=" << (trigLane.isLocked(i) ? 1 : 0) << " note=" << static_cast<int>(note.note)
           << " vel=" << static_cast<int>(note.velocity)
           << " gate=" << static_cast<int>(note.gateSteps) << "\n";
    }

    for (std::size_t laneIdx = 0; laneIdx < pattern_.laneCount(); laneIdx++)
    {
      const CcLockLane& lockLane = pattern_.lane(laneIdx);
      file << "LANE cc=" << static_cast<int>(lockLane.param.cc) << " name=" << lockLane.param.name
           << "\n";
      for (std::size_t i = 0; i < pattern_.stepCount(); i++)
      {
        if (!lockLane.lane.isLocked(i))
        {
          continue;
        }
        file << i << " value=" << static_cast<int>(lockLane.lane.lockedValue(i)) << "\n";
      }
    }

    return true;
  }

  // Loads step data into an already-configured Pattern (its CC lock lanes
  // must already exist, e.g. via Pattern::setLanes() from the same/a
  // compatible profile - lanes in the file that have no matching CC in the
  // pattern are silently skipped).
  static bool loadFromFile(const std::string& filepath_, Pattern& pattern_)
  {
    std::ifstream file(filepath_);
    if (!file.is_open())
    {
      return false;
    }

    std::string line;
    CcLockLane* currentLane = nullptr;
    bool inTrig = false;

    while (std::getline(file, line))
    {
      if (line.empty())
      {
        continue;
      }

      if (line.compare(0, 6, "steps=") == 0)
      {
        pattern_.setStepCount(static_cast<std::size_t>(std::stoul(line.substr(6))));
        continue;
      }

      if (line == "TRIG")
      {
        inTrig = true;
        currentLane = nullptr;
        continue;
      }

      if (line.compare(0, 8, "LANE cc=") == 0)
      {
        inTrig = false;
        currentLane = findLaneByCc(pattern_, line.substr(8));
        continue;
      }

      std::istringstream row(line);
      std::size_t index = 0;
      row >> index;

      if (inTrig)
      {
        loadTrigRow(row, pattern_.trigLane(), index);
      }
      else if (currentLane != nullptr)
      {
        loadLaneRow(row, *currentLane, index);
      }
    }

    return true;
  }

private:
  static CcLockLane* findLaneByCc(Pattern& pattern_, const std::string& header_)
  {
    std::istringstream headerStream(header_);
    int cc = 0;
    headerStream >> cc;

    for (std::size_t laneIdx = 0; laneIdx < pattern_.laneCount(); laneIdx++)
    {
      if (pattern_.lane(laneIdx).param.cc == static_cast<uint8_t>(cc))
      {
        return &pattern_.lane(laneIdx);
      }
    }
    return nullptr;
  }

  static void loadTrigRow(std::istringstream& row_, TrigLane& trigLane_, std::size_t index_)
  {
    bool active = false;
    bool locked = false;
    int note = 60;
    int vel = 100;
    int gate = 1;

    std::string tok;
    while (row_ >> tok)
    {
      parseBool(tok, "active=", active);
      parseBool(tok, "locked=", locked);
      parseInt(tok, "note=", note);
      parseInt(tok, "vel=", vel);
      parseInt(tok, "gate=", gate);
    }

    trigLane_.setActive(index_, active);
    if (locked)
    {
      NoteEvent ev;
      ev.note = static_cast<uint8_t>(note);
      ev.velocity = static_cast<uint8_t>(vel);
      ev.gateSteps = static_cast<uint8_t>(gate);
      trigLane_.setLock(index_, ev);
    }
  }

  static void loadLaneRow(std::istringstream& row_, CcLockLane& lockLane_, std::size_t index_)
  {
    int value = 0;
    std::string tok;
    while (row_ >> tok)
    {
      parseInt(tok, "value=", value);
    }
    lockLane_.lane.setLock(index_, static_cast<uint8_t>(value));
  }

  static void parseBool(const std::string& tok_, const char* key_, bool& out_)
  {
    const std::string key(key_);
    if (tok_.compare(0, key.size(), key) == 0)
    {
      out_ = tok_.substr(key.size()) == "1";
    }
  }

  static void parseInt(const std::string& tok_, const char* key_, int& out_)
  {
    const std::string key(key_);
    if (tok_.compare(0, key.size(), key) == 0)
    {
      out_ = std::stoi(tok_.substr(key.size()));
    }
  }
};

} // namespace seq
} // namespace sl

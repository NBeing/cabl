/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include <cabl/cabl.h>

#include "core/Pattern.h"
#include "core/SynthProfile.h"
#include "desktop/PatchNamesLoader.h"
#include "desktop/ThreadedStepClock.h"

namespace sl
{

using namespace cabl;
using namespace seq;

// Digitakt-style 16-step sequencer for the MK1: 16 pads = 16 steps of a
// single trig lane, 8 knob encoders = 8 live CC lanes (paged) drawn from a
// loaded SynthProfile (e.g. profiles/hydrasynth.ccmap). Tap a pad to toggle
// its trig; hold a pad and turn an encoder to write a per-step lock for
// that lane - unlocked steps simply don't touch that CC when they play.
class PlockSeq : public Client
{
public:
  PlockSeq(uint8_t channel_,
    const SynthProfile& profile_,
    const std::string& patternFile_,
    const std::string& patchNamesFile_ = "",
    double bpm_ = 120.0);

  void initDevice() override;
  void render() override;
  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftPressed_) override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;
  void keyChanged(unsigned index_, double value_, bool shiftPressed_) override;

private:
  static constexpr unsigned kPadsPerPage = 16;
  static constexpr unsigned kLanesPerPage = 8;
  static constexpr unsigned kFirstLaneEncoder = 1; // encoders 1-8 = the 8 knob encoders
  static constexpr unsigned kTempoEncoder = 0;
  static constexpr int kMinBpm = 40;
  static constexpr int kMaxBpm = 240;

  void onStep(std::size_t stepIndex_);
  void togglePlay();
  void sendNotePulse(const NoteEvent& note_);
  void sendCc(uint8_t cc_, uint8_t value_);
  void savePattern();
  void loadPattern();
  void setStepPage(int page_);
  void setLanePage(int page_);
  std::size_t stepPageCount() const;
  std::size_t lanePageCount() const;
  bool stepHasAnyLock(std::size_t step_) const;

  void drawStepGrid(Canvas* canvas_);
  void drawLanes(Canvas* canvas_);
  void updatePadLeds();

  uint8_t m_channel;
  std::string m_patternFile;
  SynthProfile m_profile;
  Pattern m_pattern;
  ThreadedStepClock m_clock;

  double m_bpm;
  bool m_playing{false};
  std::atomic<std::size_t> m_currentStep{0};

  std::size_t m_stepPage{0};
  std::size_t m_lanePage{0};

  int m_heldStep{-1};
  bool m_lockWrittenDuringHold{false};
  bool m_sceneHeld{false};

  NoteEvent m_defaultNote{};

  bool m_dirty{true};

  PatchNamesLoader::PatchDatabase m_patches{};
  bool m_hasPatches{false};
};

} // namespace sl

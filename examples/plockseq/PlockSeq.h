/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cstdint>
#include <string>

#include <cabl/cabl.h>
#include <cabl/threading/RTSafeEventDistributor.h>

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
//
// Two threads touch this class: the Coordinator's device thread (button/
// encoder/pad callbacks and render(), all serialized on one thread per
// Client.h) and ThreadedStepClock's own clock thread (onStep()). Pattern
// itself is safe to read from one and write from the other field-by-field
// (see StepLane.h), but "this step fired" needs to reach the UI thread's
// render logic without the clock thread reaching into UI-owned state
// directly - that's what RTSafeEventDistributor (already built for exactly
// this RT/UI split, see cabl/inc/cabl/threading/) is for here: onStep()
// only sends MIDI and enqueues a ClockTick; handleUIEvent(), drained once
// per render() on the device thread, is the only place m_currentStep/
// m_dirty get written from a step firing.
class PlockSeq
  : public Client
  , public UIObserver
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

  // UIObserver: called synchronously from processUIEvents(), i.e. from
  // render() on the device thread - never from the clock thread.
  void handleUIEvent(const RTEvent& event_) override;

private:
  static constexpr unsigned kPadsPerPage = 16;
  static constexpr unsigned kLanesPerPage = 8;
  static constexpr unsigned kFirstLaneEncoder = 1; // encoders 1-8 = the 8 knob encoders
  static constexpr unsigned kTempoEncoder = 0;
  static constexpr int kMinBpm = 40;
  static constexpr int kMaxBpm = 240;

  void onStep(std::size_t stepIndex_); // clock thread
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

  // Bulk pattern edits (save/load/length-change) touch several lanes/
  // fields at once, which per-field atomics don't make safe against a
  // concurrently-running clock thread - refuse them while playing instead,
  // logging why. Returns true if the operation should be refused.
  bool refuseWhilePlaying(const char* what_) const;

  void drawStepGrid(Canvas* canvas_);
  void drawLanes(Canvas* canvas_);
  void updatePadLeds();

  uint8_t m_channel;
  std::string m_patternFile;
  SynthProfile m_profile;
  Pattern m_pattern;
  // Declared in this order deliberately: members are destroyed in reverse
  // declaration order, so m_clock (declared last of the two) is destroyed
  // - and its thread joined, by ThreadedStepClock's own destructor -
  // before m_distributor is destroyed. That guarantees the clock thread
  // can never touch a half- or fully-destroyed m_distributor (or
  // m_pattern, declared even earlier and therefore destroyed even later).
  RTSafeEventDistributor m_distributor;
  ThreadedStepClock m_clock;

  double m_bpm;
  bool m_playing{false};
  std::size_t m_currentStep{0}; // UI-thread only - written from handleUIEvent()

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

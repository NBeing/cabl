/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "PlockSeq.h"

#include <algorithm>
#include <iostream>

#include "desktop/PatternFileLoader.h"

namespace
{
// The MK1's button/pad LEDs take a 7-bit brightness value - 128 or higher
// doesn't clip to max, it misbehaves (confirmed empirically, see
// cabl/examples/buttondebug/Mk1Led.h). Canvas pixel colors (putText/
// rectangle on the graphic displays) aren't affected, only setKeyLed/
// setButtonLed.
constexpr uint8_t kMaxLed = 127;
} // namespace

namespace sl
{

// Out-of-class definitions for static constexpr members that get
// ODR-used by reference (e.g. via std::min/std::max) - required pre-C++17.
constexpr int PlockSeq::kMinBpm;
constexpr int PlockSeq::kMaxBpm;

//--------------------------------------------------------------------------------------------------

PlockSeq::PlockSeq(uint8_t channel_,
  const SynthProfile& profile_,
  const std::string& patternFile_,
  const std::string& patchNamesFile_,
  double bpm_)
  : m_channel(channel_)
  , m_patternFile(patternFile_)
  , m_profile(profile_)
  , m_pattern(kPadsPerPage)
  , m_clock(kPadsPerPage)
  , m_bpm(bpm_)
{
  m_pattern.setLanes(m_profile.params);
  m_clock.setOnStep([this](std::size_t step_) { onStep(step_); });

  if (!patchNamesFile_.empty())
  {
    m_hasPatches = PatchNamesLoader::loadFromFile(patchNamesFile_, m_patches);
  }
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::initDevice()
{
  m_clock.setStepCount(m_pattern.stepCount());

  std::cout << "Connected. Sending on MIDI channel " << static_cast<int>(m_channel) << ".\n"
            << "Synth profile: " << m_profile.name << " (" << m_profile.params.size()
            << " params)\n"
            << "Pads: tap = toggle step on/off.\n"
            << "Pads: hold pad + turn an encoder = lock that step's value on that lane.\n"
            << "Encoders 1-8: live CC value for the 8 lanes on the current lane page (hold\n"
            << "  Shift while turning for fine steps).\n"
            << "Main encoder: tempo (BPM), hold Shift for fine steps.\n"
            << "BrowseLeft/BrowseRight: switch lane page.\n"
            << "PageLeft/PageRight: switch step page.\n"
            << "GroupA-D: set pattern length to 16/32/48/64 steps.\n"
            << "Play: start/stop playback.\n"
            << "Scene+Play: save pattern. Scene+Erase: load pattern.\n"
            << "Pattern file: " << m_patternFile << "\n"
            << "Type 'q' and hit ENTER to quit." << std::endl;

  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::buttonChanged(Device::Button button_, bool buttonState_, bool shiftPressed_)
{
  if (button_ == Device::Button::Scene)
  {
    m_sceneHeld = buttonState_;
    return;
  }

  if (!buttonState_)
  {
    return; // everything below acts on press only
  }

  if (button_ == Device::Button::Play)
  {
    if (m_sceneHeld)
    {
      savePattern();
    }
    else
    {
      togglePlay();
    }
    return;
  }

  if (button_ == Device::Button::Erase && m_sceneHeld)
  {
    loadPattern();
    return;
  }

  if (button_ == Device::Button::PageLeft)
  {
    setStepPage(static_cast<int>(m_stepPage) - 1);
    return;
  }

  if (button_ == Device::Button::PageRight)
  {
    setStepPage(static_cast<int>(m_stepPage) + 1);
    return;
  }

  if (button_ == Device::Button::BrowseLeft)
  {
    setLanePage(static_cast<int>(m_lanePage) - 1);
    return;
  }

  if (button_ == Device::Button::BrowseRight)
  {
    setLanePage(static_cast<int>(m_lanePage) + 1);
    return;
  }

  if (button_ >= Device::Button::GroupA && button_ <= Device::Button::GroupD)
  {
    const uint8_t idx
      = static_cast<uint8_t>(button_) - static_cast<uint8_t>(Device::Button::GroupA);
    m_pattern.setStepCount((idx + 1) * kPadsPerPage);
    m_clock.setStepCount(m_pattern.stepCount());
    if (m_stepPage >= stepPageCount())
    {
      setStepPage(0);
    }
    m_dirty = true;
    requestDeviceUpdate();
    return;
  }

  (void)shiftPressed_;
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_)
{
  if (encoder_ == kTempoEncoder)
  {
    const int step = shiftPressed_ ? 1 : 5;
    int next = static_cast<int>(m_bpm) + (valueIncreased_ ? step : -step);
    next = std::max(kMinBpm, std::min(kMaxBpm, next));
    m_bpm = static_cast<double>(next);
    m_clock.setBpm(m_bpm);
    m_dirty = true;
    requestDeviceUpdate();
    return;
  }

  if (encoder_ < kFirstLaneEncoder || encoder_ >= kFirstLaneEncoder + kLanesPerPage)
  {
    return;
  }

  const std::size_t slot = encoder_ - kFirstLaneEncoder;
  const std::size_t laneIndex = m_lanePage * kLanesPerPage + slot;
  if (laneIndex >= m_pattern.laneCount())
  {
    return;
  }

  CcLockLane& lockLane = m_pattern.lane(laneIndex);
  const int step = shiftPressed_ ? 1 : 4;
  int next = static_cast<int>(lockLane.liveValue) + (valueIncreased_ ? step : -step);
  lockLane.liveValue = static_cast<uint8_t>(std::max(0, std::min(127, next)));

  if (m_heldStep >= 0)
  {
    lockLane.lane.setLock(static_cast<std::size_t>(m_heldStep), lockLane.liveValue);
    m_lockWrittenDuringHold = true;
  }

  sendCc(lockLane.param.cc, lockLane.liveValue);

  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::keyChanged(unsigned index_, double value_, bool /*shiftPressed_*/)
{
  if (index_ >= kPadsPerPage)
  {
    return;
  }

  const std::size_t absStep = m_stepPage * kPadsPerPage + index_;
  if (absStep >= m_pattern.stepCount())
  {
    return;
  }

  if (value_ > 0.0)
  {
    m_heldStep = static_cast<int>(absStep);
    m_lockWrittenDuringHold = false;
  }
  else if (m_heldStep == static_cast<int>(absStep))
  {
    if (!m_lockWrittenDuringHold)
    {
      m_pattern.trigLane().toggleActive(absStep);
    }
    m_heldStep = -1;
  }

  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::togglePlay()
{
  m_playing = !m_playing;
  if (m_playing)
  {
    m_clock.setStepCount(m_pattern.stepCount());
    m_clock.start(m_bpm);
  }
  else
  {
    m_clock.stop();
  }
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::onStep(std::size_t stepIndex_)
{
  m_currentStep = stepIndex_;

  for (std::size_t laneIdx = 0; laneIdx < m_pattern.laneCount(); laneIdx++)
  {
    CcLockLane& lockLane = m_pattern.lane(laneIdx);
    if (lockLane.lane.isLocked(stepIndex_))
    {
      sendCc(lockLane.param.cc, lockLane.lane.lockedValue(stepIndex_));
    }
  }

  if (m_pattern.trigLane().isActive(stepIndex_))
  {
    const NoteEvent note = m_pattern.trigLane().isLocked(stepIndex_)
      ? m_pattern.trigLane().lockedValue(stepIndex_)
      : m_defaultNote;
    sendNotePulse(note);
  }

  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::sendNotePulse(const NoteEvent& note_)
{
  // Instant on/off pulse, same technique Euklid::play() uses - no gate
  // timer needed, and it's audible on real gear. gateSteps is reserved for
  // a future multi-step-sustain feature.
  const uint8_t channelNibble = static_cast<uint8_t>((m_channel - 1) & 0x0F);
  device()->sendMidiMsg({static_cast<uint8_t>(0x90 | channelNibble), note_.note, note_.velocity});
  device()->sendMidiMsg({static_cast<uint8_t>(0x80 | channelNibble), note_.note, 0});
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::sendCc(uint8_t cc_, uint8_t value_)
{
  const uint8_t channelNibble = static_cast<uint8_t>((m_channel - 1) & 0x0F);
  device()->sendMidiMsg({static_cast<uint8_t>(0xB0 | channelNibble), cc_, value_});
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::savePattern()
{
  if (PatternFileLoader::saveToFile(m_patternFile, m_pattern))
  {
    std::cout << "Saved pattern -> " << m_patternFile << std::endl;
  }
  else
  {
    std::cout << "Failed to save pattern to " << m_patternFile << std::endl;
  }
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::loadPattern()
{
  if (PatternFileLoader::loadFromFile(m_patternFile, m_pattern))
  {
    m_clock.setStepCount(m_pattern.stepCount());
    std::cout << "Loaded pattern <- " << m_patternFile << std::endl;
  }
  else
  {
    std::cout << "Failed to load pattern from " << m_patternFile << std::endl;
  }
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

std::size_t PlockSeq::stepPageCount() const
{
  return (m_pattern.stepCount() + kPadsPerPage - 1) / kPadsPerPage;
}

//--------------------------------------------------------------------------------------------------

std::size_t PlockSeq::lanePageCount() const
{
  if (m_pattern.laneCount() == 0)
  {
    return 1;
  }
  return (m_pattern.laneCount() + kLanesPerPage - 1) / kLanesPerPage;
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::setStepPage(int page_)
{
  const int count = static_cast<int>(stepPageCount());
  page_ = ((page_ % count) + count) % count;
  m_stepPage = static_cast<std::size_t>(page_);
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::setLanePage(int page_)
{
  const int count = static_cast<int>(lanePageCount());
  page_ = ((page_ % count) + count) % count;
  m_lanePage = static_cast<std::size_t>(page_);
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

bool PlockSeq::stepHasAnyLock(std::size_t step_) const
{
  if (m_pattern.trigLane().isLocked(step_))
  {
    return true;
  }
  for (std::size_t laneIdx = 0; laneIdx < m_pattern.laneCount(); laneIdx++)
  {
    if (m_pattern.lane(laneIdx).lane.isLocked(step_))
    {
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::render()
{
  if (!m_dirty)
  {
    return;
  }
  m_dirty = false;

  Canvas* d0 = device()->graphicDisplay(0);
  d0->black();
  drawStepGrid(d0);

  Canvas* d1 = device()->graphicDisplay(1);
  d1->black();
  drawLanes(d1);

  updatePadLeds();

  device()->setButtonLed(Device::Button::Play, m_playing ? Color(kMaxLed) : Color(0x00));
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::drawStepGrid(Canvas* canvas_)
{
  constexpr const char* kFont = "small";
  const std::size_t visibleStep = m_currentStep.load();
  const bool onScreenPlayhead = m_playing && (visibleStep / kPadsPerPage == m_stepPage);

  const std::string header
    = "PLOCKSEQ  " + std::to_string(static_cast<int>(m_bpm)) + " bpm" + (m_playing ? " >" : " .");
  canvas_->putText(4, 2, header.c_str(), {0xff}, kFont, 0);

  const std::string pageLine = "steps " + std::to_string(m_pattern.stepCount()) + "  step-pg "
    + std::to_string(m_stepPage + 1) + "/" + std::to_string(stepPageCount()) + "  lane-pg "
    + std::to_string(m_lanePage + 1) + "/" + std::to_string(lanePageCount());
  canvas_->putText(4, 12, pageLine.c_str(), {0xff}, kFont, 0);

  constexpr int kBoxSize = 12;
  constexpr int kBoxGap = 3;
  constexpr int kGridY = 26;

  for (unsigned i = 0; i < kPadsPerPage; i++)
  {
    const std::size_t absStep = m_stepPage * kPadsPerPage + i;
    if (absStep >= m_pattern.stepCount())
    {
      continue;
    }

    const int x = 4 + static_cast<int>(i) * (kBoxSize + kBoxGap);
    const bool active = m_pattern.trigLane().isActive(absStep);
    const bool hasLock = stepHasAnyLock(absStep);
    const bool isPlayhead = onScreenPlayhead && (absStep == visibleStep);

    if (active)
    {
      canvas_->rectangleFilled(x, kGridY, kBoxSize, kBoxSize, {0xff}, {0xff});
    }
    else
    {
      canvas_->rectangle(x, kGridY, kBoxSize, kBoxSize, {0xff});
    }

    if (hasLock)
    {
      const Color dot = active ? Color(0x00) : Color(0xff);
      canvas_->rectangleFilled(
        x + kBoxSize / 2 - 1, kGridY + kBoxSize / 2 - 1, 2, 2, dot, dot);
    }

    if (isPlayhead)
    {
      canvas_->rectangle(x - 1, kGridY - 1, kBoxSize + 2, kBoxSize + 2, {BlendMode::Invert});
    }
  }
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::drawLanes(Canvas* canvas_)
{
  constexpr const char* kFont = "small";
  constexpr int kLineStep = 7;

  const std::size_t base = m_lanePage * kLanesPerPage;
  for (unsigned slot = 0; slot < kLanesPerPage; slot++)
  {
    const std::size_t laneIndex = base + slot;
    if (laneIndex >= m_pattern.laneCount())
    {
      break;
    }

    const CcLockLane& lockLane = m_pattern.lane(laneIndex);
    const bool heldStepLocked
      = (m_heldStep >= 0) && lockLane.lane.isLocked(static_cast<std::size_t>(m_heldStep));

    const std::string line = std::string(heldStepLocked ? "*" : " ") + std::to_string(slot + 1)
      + " " + lockLane.param.name + " " + std::to_string(lockLane.liveValue);
    canvas_->putText(2, 1 + static_cast<int>(slot) * kLineStep, line.c_str(), {0xff}, kFont, 0);
  }
}

//--------------------------------------------------------------------------------------------------

void PlockSeq::updatePadLeds()
{
  const Color kInactive(0x00);
  const Color kInactiveLocked(0x20);
  const Color kActive(0x60);
  const Color kActiveLocked(kMaxLed);
  const Color kHeld(kMaxLed);
  const Color kPlayheadColor(0x40);

  const std::size_t visibleStep = m_currentStep.load();
  const bool onScreenPlayhead = m_playing && (visibleStep / kPadsPerPage == m_stepPage);

  for (unsigned i = 0; i < kPadsPerPage; i++)
  {
    const std::size_t absStep = m_stepPage * kPadsPerPage + i;
    if (absStep >= m_pattern.stepCount())
    {
      device()->setKeyLed(i, kInactive);
      continue;
    }

    if (static_cast<int>(absStep) == m_heldStep)
    {
      device()->setKeyLed(i, kHeld);
      continue;
    }

    const bool active = m_pattern.trigLane().isActive(absStep);
    const bool locked = stepHasAnyLock(absStep);

    Color c = active ? (locked ? kActiveLocked : kActive) : (locked ? kInactiveLocked : kInactive);
    if (onScreenPlayhead && absStep == visibleStep)
    {
      c = kPlayheadColor;
    }
    device()->setKeyLed(i, c);
  }
}

} // namespace sl

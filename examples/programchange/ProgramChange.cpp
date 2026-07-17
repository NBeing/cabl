/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "ProgramChange.h"

#include <algorithm>
#include <array>
#include <iostream>

namespace
{

using ParamDef = sl::ProgramChange::ParamDef;

constexpr std::array<ParamDef, 32> kHydraParams{{
  {74, "F1 Cutoff"},
  {71, "F1 Resonance"},
  {50, "F1 Drive"},
  {55, "F2 Cutoff"},
  {56, "F2 Resonance"},
  {44, "OSC1 Volume"},
  {45, "OSC1 Pan"},
  {24, "OSC1 Wavscan"},
  {46, "OSC2 Volume"},
  {47, "OSC2 Pan"},
  {26, "OSC2 Wavscan"},
  {48, "OSC3 Volume"},
  {49, "OSC3 Pan"},
  {81, "ENV1 Attack"},
  {82, "ENV1 Decay"},
  {83, "ENV1 Sustain"},
  {84, "ENV1 Release"},
  {72, "LFO1 Rate"},
  {70, "LFO1 Gain"},
  {73, "LFO2 Rate"},
  {28, "LFO2 Gain"},
  {16, "Macro 1"},
  {17, "Macro 2"},
  {18, "Macro 3"},
  {19, "Macro 4"},
  {20, "Macro 5"},
  {21, "Macro 6"},
  {22, "Macro 7"},
  {23, "Macro 8"},
  {91, "Reverb Mix"},
  {92, "Delay Mix"},
  {117, "Stereo Width"},
}};

} // namespace

namespace sl
{

//--------------------------------------------------------------------------------------------------

ProgramChange::ProgramChange(uint8_t channel_, const std::string& patchNamesFile_, Mode mode_)
  : m_channel(channel_), m_mode(mode_)
{
  // Try to load patch names if file is provided
  if (!patchNamesFile_.empty())
  {
    if (!PatchNamesLoader::loadFromFile(patchNamesFile_, m_patches))
    {
      std::cerr << "Warning: Could not load patch names from " << patchNamesFile_ << std::endl;
    }
  }

  m_browserBank = m_bank;
  m_browserProgram = m_program;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::initDevice()
{
  std::cout << "Connected. Sending on MIDI channel " << static_cast<int>(m_channel) << ".\n"
            << "Mode: press Pattern to toggle Program/Parameter mode.\n"
            << "Program mode: A-E choose bank; encoder #1 chooses program.\n"
            << "Program browse: tap Browse (or Navigate) to enter/exit browser; encoder #1 scrolls preview patches.\n"
            << "Program browse: BrowseLeft/BrowseRight step preview directly on MK1.\n"
            << "Program browse: press Select/Play to confirm and send the previewed patch.\n"
            << "Parameter mode: encoder #1 chooses parameter; encoder #2 chooses value.\n"
            << "Parameter step: coarse by default, hold Shift for fine changes.\n"
            << "Parameter value changes are sent immediately as CC messages.\n"
            << "Pads: hold Scene + hit pad to STORE current target.\n"
            << "Pads: hit pad (without Scene) to RECALL and send instantly.\n"
            << "Send: Press Select or Play to send current Program/CC.\n"
            << "Test: Hold Erase to send a test Note On/Off (note " << static_cast<int>(kTestNote)
            << ") - basic sanity check.\n"
            << "Type 'q' and hit ENTER to quit." << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_)
{
  if (m_mode == Mode::Program)
  {
    if (encoder_ != kEncoderIndex)
    {
      return;
    }

    if (m_patchBrowserActive)
    {
      advancePatchBrowser(valueIncreased_);
    }
    else
    {
      int next = static_cast<int>(m_program) + (valueIncreased_ ? 1 : -1);
      m_program = static_cast<uint8_t>(std::max(0, std::min(127, next)));
    }
  }
  else
  {
    if (encoder_ == kParamSelectEncoder)
    {
      int next = static_cast<int>(m_paramIndex) + (valueIncreased_ ? 1 : -1);
      int maxIndex = static_cast<int>(kHydraParams.size() - 1);
      m_paramIndex = static_cast<size_t>(std::max(0, std::min(maxIndex, next)));
    }
    else if (encoder_ == kParamValueEncoder)
    {
      int step = shiftPressed_ ? static_cast<int>(kParamFineStep) : static_cast<int>(kParamCoarseStep);
      int next = static_cast<int>(m_paramValue) + (valueIncreased_ ? step : -step);
      m_paramValue = static_cast<uint8_t>(std::max(0, std::min(127, next)));
      sendCurrentParameter();
    }
    else
    {
      return;
    }
  }

  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::buttonChanged(Device::Button button_, bool buttonState_, bool /*shiftPressed_*/)
{
  if (button_ == Device::Button::Pattern && buttonState_)
  {
    if (m_patchBrowserActive)
    {
      setPatchBrowserActive(false);
    }
    m_mode = (m_mode == Mode::Program) ? Mode::Parameter : Mode::Program;
    m_dirty = true;
    requestDeviceUpdate();
    std::cout << "Mode -> " << (m_mode == Mode::Program ? "Program" : "Parameter") << std::endl;
    return;
  }

  if ((button_ == Device::Button::Browse || button_ == Device::Button::Navigate) && buttonState_)
  {
    if (m_mode == Mode::Program)
    {
      setPatchBrowserActive(!m_patchBrowserActive);
      std::cout << "Patch browser -> " << (m_patchBrowserActive ? "ON" : "OFF") << std::endl;
    }
    return;
  }

  if (m_mode == Mode::Program && m_patchBrowserActive && buttonState_)
  {
    if (button_ == Device::Button::BrowseLeft)
    {
      advancePatchBrowser(false);
      m_dirty = true;
      requestDeviceUpdate();
      return;
    }
    if (button_ == Device::Button::BrowseRight)
    {
      advancePatchBrowser(true);
      m_dirty = true;
      requestDeviceUpdate();
      return;
    }
  }

  if (button_ == Device::Button::Scene)
  {
    m_sceneHeld = buttonState_;
    m_dirty = true;
    requestDeviceUpdate();
    return;
  }

  if (!buttonState_)
  {
    // Only act on button press, not release
    if (button_ == Device::Button::Erase)
    {
      sendTestNote(false);
    }
    return;
  }

  // Bank selection via Group A-E buttons
  if (m_mode == Mode::Program && !m_patchBrowserActive
    && button_ >= Device::Button::GroupA && button_ <= Device::Button::GroupE)
  {
    uint8_t buttonVal = static_cast<uint8_t>(button_);
    uint8_t groupAVal = static_cast<uint8_t>(Device::Button::GroupA);
    uint8_t bankIndex = buttonVal - groupAVal;
    
    if (bankIndex <= kMaxBank)
    {
      m_bank = bankIndex;
      m_dirty = true;
      requestDeviceUpdate();
      char bankLetter = static_cast<char>('A' + bankIndex);
      std::cout << "Selected bank " << bankLetter << std::endl;
      return;
    }
  }

  // Send program on Select or Play
  if (button_ == Device::Button::Select || button_ == Device::Button::Play)
  {
    if (m_mode == Mode::Program)
    {
      if (m_patchBrowserActive)
      {
        m_bank = m_browserBank;
        m_program = m_browserProgram;
        setPatchBrowserActive(false);
      }
      sendCurrentProgram();
    }
    else
    {
      sendCurrentParameter();
    }
  }
  // Test note on Erase press
  else if (button_ == Device::Button::Erase)
  {
    sendTestNote(true);
  }

  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::setPatchBrowserActive(bool active_)
{
  m_patchBrowserActive = active_;
  if (m_patchBrowserActive)
  {
    m_browserBank = m_bank;
    m_browserProgram = m_program;
  }
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::keyChanged(unsigned index_, double value_, bool /*shiftPressed_*/)
{
  // Treat a positive pad value as a pad hit, ignore release/aftertouch-like zeros.
  if (index_ >= kNumPads || value_ <= 0.0)
  {
    return;
  }

  if (m_sceneHeld)
  {
    if (m_mode == Mode::Program)
    {
      PadPreset& slot = m_padPresets[index_];
      slot.assigned = true;
      slot.bank = m_bank;
      slot.program = m_program;
    }
    else
    {
      ParamPreset& slot = m_paramPresets[index_];
      slot.assigned = true;
      slot.cc = selectedParam().cc;
      slot.value = m_paramValue;
    }

    m_lastPadSlot = static_cast<int>(index_);
    m_lastPadActionWasStore = true;
    m_dirty = true;
    requestDeviceUpdate();
    if (m_mode == Mode::Program)
    {
      std::cout << "Stored pad " << (index_ + 1) << " -> "
                << static_cast<char>('A' + m_bank) << static_cast<int>(m_program) << std::endl;
    }
    else
    {
      std::cout << "Stored pad " << (index_ + 1) << " -> CC "
                << static_cast<int>(selectedParam().cc) << " = " << static_cast<int>(m_paramValue)
                << std::endl;
    }
    return;
  }

  bool assigned = (m_mode == Mode::Program) ? m_padPresets[index_].assigned : m_paramPresets[index_].assigned;
  if (!assigned)
  {
    m_lastPadSlot = static_cast<int>(index_);
    m_lastPadActionWasStore = false;
    m_dirty = true;
    requestDeviceUpdate();
    std::cout << "Pad " << (index_ + 1) << " is empty (hold Scene + pad to store)." << std::endl;
    return;
  }

  if (m_mode == Mode::Program)
  {
    const PadPreset& slot = m_padPresets[index_];
    m_bank = slot.bank;
    m_program = slot.program;
  }
  else
  {
    const ParamPreset& slot = m_paramPresets[index_];
    for (size_t i = 0; i < kHydraParams.size(); i++)
    {
      if (kHydraParams[i].cc == slot.cc)
      {
        m_paramIndex = i;
        break;
      }
    }
    m_paramValue = slot.value;
  }
  m_lastPadSlot = static_cast<int>(index_);
  m_lastPadActionWasStore = false;

  if (m_mode == Mode::Program)
  {
    sendCurrentProgram();
  }
  else
  {
    sendCurrentParameter();
  }
  requestDeviceUpdate();

  if (m_mode == Mode::Program)
  {
    std::cout << "Recalled pad " << (index_ + 1) << " -> "
              << static_cast<char>('A' + m_bank) << static_cast<int>(m_program) << std::endl;
  }
  else
  {
    std::cout << "Recalled pad " << (index_ + 1) << " -> CC "
              << static_cast<int>(selectedParam().cc) << " = " << static_cast<int>(m_paramValue)
              << std::endl;
  }
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::advancePatchBrowser(bool valueIncreased_)
{
  constexpr int kBankCount = static_cast<int>(kMaxBank) + 1;
  constexpr int kProgramsPerBank = 128;
  constexpr int kTotalSlots = kBankCount * kProgramsPerBank;

  int current = static_cast<int>(m_browserBank) * kProgramsPerBank + static_cast<int>(m_browserProgram);
  int step = valueIncreased_ ? 1 : -1;

  bool hasAnyNames = false;
  for (int bank = 0; bank < kBankCount && !hasAnyNames; bank++)
  {
    for (int program = 0; program < kProgramsPerBank; program++)
    {
      if (!PatchNamesLoader::getPatchNameString(
             m_patches, static_cast<uint8_t>(bank), static_cast<uint8_t>(program)).empty())
      {
        hasAnyNames = true;
        break;
      }
    }
  }

  for (int i = 1; i <= kTotalSlots; i++)
  {
    int candidate = (current + (step * i)) % kTotalSlots;
    if (candidate < 0)
    {
      candidate += kTotalSlots;
    }

    uint8_t bank = static_cast<uint8_t>(candidate / kProgramsPerBank);
    uint8_t program = static_cast<uint8_t>(candidate % kProgramsPerBank);
    const std::string& name = PatchNamesLoader::getPatchNameString(m_patches, bank, program);

    // If a patch list is loaded, browse only named patches.
    if (!hasAnyNames || !name.empty())
    {
      m_browserBank = bank;
      m_browserProgram = program;
      return;
    }
  }

  // Fallback: linear step when no named patch was found.
  int fallback = current + step;
  fallback = std::max(0, std::min(kTotalSlots - 1, fallback));
  m_browserBank = static_cast<uint8_t>(fallback / kProgramsPerBank);
  m_browserProgram = static_cast<uint8_t>(fallback % kProgramsPerBank);
}

//--------------------------------------------------------------------------------------------------

const ProgramChange::ParamDef& ProgramChange::selectedParam() const
{
  return kHydraParams[m_paramIndex];
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::sendCurrentProgram()
{
  uint8_t channelNibble = static_cast<uint8_t>((m_channel - 1) & 0x0F);

  // Full Bank Select MSB+LSB pair, then Program Change. The Hydrasynth's LSB
  // (CC 32) value is what actually picks the bank - MSB (CC 0) is reportedly
  // ignored - but some receivers won't commit *any* bank change until both
  // halves of the pair have arrived, so MSB is still sent (value 0) rather
  // than skipped, in case an LSB-only message was being read as incomplete.
  device()->sendMidiMsg({static_cast<uint8_t>(0xB0 | channelNibble), 0x00, 0x00});
  device()->sendMidiMsg({static_cast<uint8_t>(0xB0 | channelNibble), kBankSelectLsbCC, m_bank});
  device()->sendMidiMsg({static_cast<uint8_t>(0xC0 | channelNibble), m_program});

  m_lastSentBank = m_bank;
  m_lastSentProgram = m_program;
  m_dirty = true;

  std::cout << "Sent Bank Select LSB " << static_cast<int>(m_bank) << " + Program Change "
            << static_cast<int>(m_program) << " on channel " << static_cast<int>(m_channel)
            << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::sendCurrentParameter()
{
  uint8_t channelNibble = static_cast<uint8_t>((m_channel - 1) & 0x0F);
  const ParamDef& param = selectedParam();
  device()->sendMidiMsg({static_cast<uint8_t>(0xB0 | channelNibble), param.cc, m_paramValue});

  m_lastSentCc = param.cc;
  m_lastSentCcValue = m_paramValue;
  m_dirty = true;

  std::cout << "Sent CC " << static_cast<int>(param.cc) << " (" << param.name << ") = "
            << static_cast<int>(m_paramValue) << " on channel " << static_cast<int>(m_channel)
            << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::sendTestNote(bool on_)
{
  // 0x90/0x80 = Note On/Off status nibble; same 0-indexed channel as Program
  // Change above. Velocity 0 for Off, matching common MIDI convention.
  uint8_t statusByte = static_cast<uint8_t>((on_ ? 0x90 : 0x80) | ((m_channel - 1) & 0x0F));
  device()->sendMidiMsg({statusByte, kTestNote, static_cast<uint8_t>(on_ ? kTestNoteVelocity : 0)});

  m_noteHeld = on_;
  m_dirty = true;

  std::cout << "Sent Note " << (on_ ? "On" : "Off") << ": channel " << static_cast<int>(m_channel)
            << ", note " << static_cast<int>(kTestNote) << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::render()
{
  if (!m_dirty)
  {
    return;
  }
  m_dirty = false;

  char bankLetter = static_cast<char>('A' + m_bank);
  const std::string& patchName = PatchNamesLoader::getPatchNameString(m_patches, m_bank, m_program);
  char previewBankLetter = static_cast<char>('A' + m_browserBank);
  const std::string& previewPatchName = PatchNamesLoader::getPatchNameString(m_patches, m_browserBank, m_browserProgram);
  constexpr const char* kFont = "small";
  constexpr int kLineStep = 10;

  const bool browserLit = (m_mode == Mode::Program) && m_patchBrowserActive;
  device()->setButtonLed(Device::Button::Browse, browserLit ? Color(0xff) : Color(0x00));
  device()->setButtonLed(Device::Button::Navigate, browserLit ? Color(0xff) : Color(0x00));

  Canvas* d0 = device()->graphicDisplay(0);
  d0->black();
  d0->putText(4, 4, m_mode == Mode::Program ? "Program Change" : "Parameter CC", {0xff}, kFont, 0);
  d0->putText(4,
    4 + kLineStep,
    (m_mode == Mode::Program && m_patchBrowserActive ? "BROWSE ACTIVE" : ("channel " + std::to_string(static_cast<int>(m_channel)))).c_str(),
    {0xff},
    kFont,
    0);
  if (m_mode == Mode::Program)
  {
    if (m_patchBrowserActive)
    {
      d0->putText(4, 4 + (kLineStep * 2),
        ("preview " + std::string(1, previewBankLetter) + " prog " + std::to_string(m_browserProgram)).c_str(),
        {0xff},
        kFont,
        0);
      d0->putText(4,
        4 + (kLineStep * 3),
        previewPatchName.empty() ? "(no patch name)" : previewPatchName.c_str(),
        {0xff},
        kFont,
        0);
      d0->putText(4,
        4 + (kLineStep * 4),
        ("current " + std::string(1, bankLetter) + " prog " + std::to_string(m_program)).c_str(),
        {0xff},
        kFont,
        0);
    }
    else
    {
      d0->putText(4, 4 + (kLineStep * 2),
        ("bank " + std::string(1, bankLetter) + " prog " + std::to_string(m_program)).c_str(),
        {0xff},
        kFont,
        0);
      d0->putText(
        4, 4 + (kLineStep * 3), patchName.empty() ? "(no patch name)" : patchName.c_str(), {0xff}, kFont, 0);
    }
  }
  else
  {
    const ParamDef& param = selectedParam();
    d0->putText(4, 4 + (kLineStep * 2),
      ("CC " + std::to_string(static_cast<int>(param.cc)) + " = " + std::to_string(m_paramValue)).c_str(),
      {0xff},
      kFont,
      0);
    d0->putText(4, 4 + (kLineStep * 3), param.name, {0xff}, kFont, 0);
  }

  Canvas* d1 = device()->graphicDisplay(1);
  d1->black();
  if (m_mode == Mode::Program && m_lastSentProgram >= 0)
  {
    char sentBankLetter = static_cast<char>('A' + m_lastSentBank);
    const std::string& sentPatchName = PatchNamesLoader::getPatchNameString(m_patches, m_lastSentBank, m_lastSentProgram);
    d1->putText(4, 4,
      ("sent: " + std::string(1, sentBankLetter) + std::to_string(m_lastSentProgram)).c_str(),
      {0xff},
      kFont,
      0);
    d1->putText(4, 4 + kLineStep, sentPatchName.c_str(), {0xff}, kFont, 0);
  }
  else if (m_mode == Mode::Parameter && m_lastSentCc >= 0)
  {
    d1->putText(4, 4,
      ("sent: CC" + std::to_string(m_lastSentCc) + "=" + std::to_string(m_lastSentCcValue)).c_str(),
      {0xff},
      kFont,
      0);
    d1->putText(4, 4 + kLineStep, selectedParam().name, {0xff}, kFont, 0);
  }
  else
  {
    d1->putText(4, 4 + kLineStep, "press Select/Play", {0xff}, kFont, 0);
  }

  d1->putText(
    4,
    4 + (kLineStep * 3),
    m_sceneHeld ? "SCENE held: tap pad to store"
                : (m_patchBrowserActive ? "Browse: enc1 scroll, Select=load" : "tap pad to recall"),
    {0xff},
    kFont,
    0);

  if (m_lastPadSlot >= 0)
  {
    std::string slotLine = std::string("pad ") + std::to_string(m_lastPadSlot + 1)
      + (m_lastPadActionWasStore ? " stored" : " used");
    d1->putText(4, 4 + (kLineStep * 4), slotLine.c_str(), {0xff}, kFont, 0);
  }
  else
  {
    d1->putText(4, 4 + (kLineStep * 4), m_noteHeld ? "note: ON" : "note: off", {0xff}, kFont, 0);
  }
}

} // namespace sl

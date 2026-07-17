/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "ProgramChange.h"

#include <algorithm>
#include <iostream>

namespace sl
{

//--------------------------------------------------------------------------------------------------

ProgramChange::ProgramChange(uint8_t channel_) : m_channel(channel_)
{
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::initDevice()
{
  std::cout << "Connected. Sending on MIDI channel " << static_cast<int>(m_channel) << ".\n"
            << "Hold Erase to send a test Note On/Off (note " << static_cast<int>(kTestNote)
            << ") - basic sanity check that MIDI is reaching the synth at all.\n"
            << "Turn encoder #" << kEncoderIndex << " to pick a program (0-127), or hold Shift and\n"
            << "turn it to pick a bank (A-E). Press Select or Play to send Bank Select + Program Change.\n"
            << "Type 'q' and hit ENTER to quit." << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_)
{
  if (encoder_ != kEncoderIndex)
  {
    return;
  }

  if (shiftPressed_)
  {
    int next = static_cast<int>(m_bank) + (valueIncreased_ ? 1 : -1);
    m_bank = static_cast<uint8_t>(std::max(0, std::min(static_cast<int>(kMaxBank), next)));
  }
  else
  {
    int next = static_cast<int>(m_program) + (valueIncreased_ ? 1 : -1);
    m_program = static_cast<uint8_t>(std::max(0, std::min(127, next)));
  }
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::buttonChanged(Device::Button button_, bool buttonState_, bool /*shiftPressed_*/)
{
  if (buttonState_ && (button_ == Device::Button::Select || button_ == Device::Button::Play))
  {
    sendCurrentProgram();
  }
  else if (button_ == Device::Button::Erase)
  {
    sendTestNote(buttonState_);
  }
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::sendCurrentProgram()
{
  uint8_t channelNibble = static_cast<uint8_t>((m_channel - 1) & 0x0F);

  // Bank Select LSB (CC 32) first, then Program Change - the Hydrasynth
  // picks patch N from whichever bank the last Bank Select LSB selected, so
  // this has to land before the Program Change to take effect on it. Bank
  // Select MSB (CC 0) is skipped: reportedly ignored by the Hydrasynth, and
  // it only has 5 banks (A-E) so the LSB alone covers the whole range.
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

  Canvas* d0 = device()->graphicDisplay(0);
  d0->black();
  d0->putText(4, 4, "Program Change", {0xff});
  d0->putText(4, 24, ("channel " + std::to_string(static_cast<int>(m_channel))).c_str(), {0xff});
  d0->putText(
    4, 44, ("bank " + std::string(1, bankLetter) + "  program " + std::to_string(m_program)).c_str(), {0xff});

  Canvas* d1 = device()->graphicDisplay(1);
  d1->black();
  if (m_lastSentProgram >= 0)
  {
    char sentBankLetter = static_cast<char>('A' + m_lastSentBank);
    d1->putText(4, 24,
      ("sent: " + std::string(1, sentBankLetter) + std::to_string(m_lastSentProgram)).c_str(), {0xff});
  }
  else
  {
    d1->putText(4, 24, "press Select", {0xff});
  }
  d1->putText(4, 44, m_noteHeld ? "note: ON" : "note: off", {0xff});
}

} // namespace sl

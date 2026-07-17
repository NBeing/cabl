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
            << "Turn encoder #" << kEncoderIndex << " to pick a program (0-127), press Select to send it.\n"
            << "Type 'q' and hit ENTER to quit." << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::encoderChanged(unsigned encoder_, bool valueIncreased_, bool /*shiftPressed_*/)
{
  if (encoder_ != kEncoderIndex)
  {
    return;
  }

  int next = static_cast<int>(m_program) + (valueIncreased_ ? 1 : -1);
  m_program = static_cast<uint8_t>(std::max(0, std::min(127, next)));
  m_dirty = true;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ProgramChange::buttonChanged(Device::Button button_, bool buttonState_, bool /*shiftPressed_*/)
{
  if (button_ == Device::Button::Select && buttonState_)
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
  // 0xC0 = Program Change status nibble; low nibble is the 0-indexed
  // channel (so the user's channel 3 becomes 2 on the wire).
  uint8_t statusByte = static_cast<uint8_t>(0xC0 | ((m_channel - 1) & 0x0F));
  device()->sendMidiMsg({statusByte, m_program});

  m_lastSentProgram = m_program;
  m_dirty = true;

  std::cout << "Sent Program Change: channel " << static_cast<int>(m_channel) << ", program "
            << static_cast<int>(m_program) << std::endl;
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

  Canvas* d0 = device()->graphicDisplay(0);
  d0->black();
  d0->putText(4, 4, "Program Change", {0xff});
  d0->putText(4, 24, ("channel " + std::to_string(static_cast<int>(m_channel))).c_str(), {0xff});
  d0->putText(4, 44, ("program " + std::to_string(static_cast<int>(m_program))).c_str(), {0xff});

  Canvas* d1 = device()->graphicDisplay(1);
  d1->black();
  d1->putText(4, 24,
    m_lastSentProgram >= 0 ? ("sent: " + std::to_string(m_lastSentProgram)).c_str() : "press Select",
    {0xff});
  d1->putText(4, 44, m_noteHeld ? "note: ON" : "note: off", {0xff});
}

} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/cabl.h>

namespace sl
{

using namespace cabl;

// Sends MIDI out the MK1's physical MIDI OUT jack, for testing against an
// external synth (e.g. an ASM Hydrasynth) without recompiling per patch:
//   - Erase (hold/release): sends Note On/Off for note 60 - the basic
//     "is any MIDI reaching the synth at all, on this channel" sanity check.
//   - Encoder #1: pick a program (0-127).
//   - Encoder #1 + Shift: pick a bank (0-4, A-E) - sent as Bank Select LSB
//     (CC 32) right before the Program Change. The Hydrasynth reportedly
//     ignores Bank Select MSB (CC 0) entirely, so that's not sent.
//   - Select or Play: send Bank Select LSB + Program Change.
class ProgramChange : public Client
{
public:
  explicit ProgramChange(uint8_t channel_);

  void initDevice() override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;
  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftPressed_) override;
  void render() override;

private:
  static constexpr unsigned kEncoderIndex = 1;
  static constexpr uint8_t kTestNote = 60;
  static constexpr uint8_t kTestNoteVelocity = 100;
  static constexpr uint8_t kBankSelectLsbCC = 32;
  static constexpr uint8_t kMaxBank = 4; // 0-4 = banks A-E

  void sendCurrentProgram();
  void sendTestNote(bool on_);

  uint8_t m_channel; // 1-16, as given by the user (converted to 0-15 nibble on send)
  uint8_t m_program{0}; // 0-127
  uint8_t m_bank{0}; // 0-4, A-E
  int m_lastSentProgram{-1}; // -1 = nothing sent yet
  int m_lastSentBank{-1};
  bool m_noteHeld{false};
  bool m_dirty{true};
};

} // namespace sl

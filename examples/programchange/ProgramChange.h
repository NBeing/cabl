/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <array>
#include <cstddef>
#include <cabl/cabl.h>
#include "PatchNamesLoader.h"

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
  enum class Mode : uint8_t
  {
    Program,
    Parameter,
  };

  struct ParamDef
  {
    uint8_t cc;
    const char* name;

    constexpr ParamDef(uint8_t cc_ = 0, const char* name_ = nullptr) : cc(cc_), name(name_)
    {
    }
  };

  explicit ProgramChange(
    uint8_t channel_, const std::string& patchNamesFile_ = "", Mode mode_ = Mode::Program);

  void initDevice() override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;
  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftPressed_) override;
  void keyChanged(unsigned index_, double value_, bool shiftPressed_) override;
  void render() override;

private:
  static constexpr unsigned kEncoderIndex = 1;
  static constexpr uint8_t kTestNote = 60;
  static constexpr uint8_t kTestNoteVelocity = 100;
  static constexpr uint8_t kBankSelectLsbCC = 32;
  static constexpr uint8_t kMaxBank = 4; // 0-4 = banks A-E
  static constexpr unsigned kNumPads = 16;
  static constexpr unsigned kParamSelectEncoder = 1;
  static constexpr unsigned kParamValueEncoder = 2;
  static constexpr unsigned kParamFineStep = 1;
  static constexpr unsigned kParamCoarseStep = 8;

  struct PadPreset
  {
    bool assigned{false};
    uint8_t bank{0};
    uint8_t program{0};
  };

  struct ParamPreset
  {
    bool assigned{false};
    uint8_t cc{0};
    uint8_t value{0};
  };

  void sendCurrentProgram();
  void sendCurrentParameter();
  void sendTestNote(bool on_);
  void advancePatchBrowser(bool valueIncreased_);
  void setPatchBrowserActive(bool active_);
  const ParamDef& selectedParam() const;

  uint8_t m_channel; // 1-16, as given by the user (converted to 0-15 nibble on send)
  uint8_t m_program{0}; // 0-127
  uint8_t m_bank{0}; // 0-4, A-E
  size_t m_paramIndex{0};
  uint8_t m_paramValue{64};
  int m_lastSentProgram{-1}; // -1 = nothing sent yet
  int m_lastSentBank{-1};
  int m_lastSentCc{-1};
  int m_lastSentCcValue{-1};
  bool m_noteHeld{false};
  bool m_dirty{true};

  std::array<PadPreset, kNumPads> m_padPresets{};
  std::array<ParamPreset, kNumPads> m_paramPresets{};
  bool m_sceneHeld{false};
  bool m_patchBrowserActive{false};
  int m_lastPadSlot{-1};
  bool m_lastPadActionWasStore{false};
  Mode m_mode{Mode::Program};
  uint8_t m_browserBank{0};
  uint8_t m_browserProgram{0};
  
  // Runtime-loaded patch names
  PatchNamesLoader::PatchDatabase m_patches;
};

} // namespace sl

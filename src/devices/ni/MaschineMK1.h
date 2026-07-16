/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <bitset>
#include <deque>
#include <memory>

#include "cabl/comm/Transfer.h"
#include "cabl/devices/Device.h"
#include "cabl/devices/DeviceFactory.h"
#include "cabl/threading/LockFreeQueue.h"
#include "gfx/displays/GDisplayMaschineMK1.h"

class RtMidiIn;
class RtMidiOut;

namespace sl
{
namespace cabl
{

//--------------------------------------------------------------------------------------------------

class MaschineMK1 : public Device
{

public:
  MaschineMK1();

  void setButtonLed(Device::Button, const Color&) override;
  void setKeyLed(unsigned, const Color&) override;

  void sendMidiMsg(tRawData) override;

  Canvas* graphicDisplay(size_t displayIndex_) override;

  size_t numOfGraphicDisplays() const override
  {
    return 2;
  }

  size_t numOfTextDisplays() const override
  {
    return 0;
  }

  size_t numOfLedMatrices() const override
  {
    return 0;
  }

  size_t numOfLedArrays() const override
  {
    return 0;
  }

  bool tick() override;

private:
  enum class Led : uint8_t;
  enum class Button : uint8_t;
  enum class Encoder : uint8_t;

  // tick()'s round-robin step. MIDI-out isn't part of this anymore - it
  // runs on its own dedicated thread (see init()) so a slow display refresh
  // can't add jitter to it.
  enum class TickStep : uint8_t
  {
    SendFrame,
    Read,
    SendLeds,
  };

  static constexpr uint8_t kMASMK1_nDisplays = 2;
  static constexpr uint8_t kMASMK1_ledsDataSize = 62;
  static constexpr uint8_t kMASMK1_nButtons = 42;
  static constexpr uint8_t kMASMK1_buttonsDataSize = 7;
  static constexpr uint8_t kMASMK1_padDataSize = 64;
  static constexpr uint8_t kMASMK1_padsBufferSize = 16;
  static constexpr uint8_t kMASMK1_nPads = 16;

  static constexpr uint8_t kMASMK1_nEncoders = 11;

  static constexpr size_t kMASMK1_midiOutQueueCapacity = 256;

  void init() override;

  void initDisplay(uint8_t displayIndex_);
  bool sendFrame(uint8_t displayIndex_);
  bool sendLeds();
  bool read();

  bool getNextMidiOutMsg(tRawData& midiMsg_);
  bool writeMidiMsg();

  void processPads(const Transfer&);
  void processButtons(const Transfer&);
  void processEncoders(const Transfer&);
  void processMidiIn(const Transfer&);

  void setLedImpl(Led, const Color&);
  Led led(Device::Button) const noexcept;
  Led led(unsigned) const noexcept;

  Device::Button deviceButton(Button btn_) const noexcept;

  void cbRead(Transfer);

  bool isButtonPressed(Button button) const noexcept;
  bool isButtonPressed(const Transfer&, Button button_) const noexcept;

  GDisplayMaschineMK1 m_displays[kMASMK1_nDisplays];
  // Consecutive sendFrame() failures per display, since a failure can be a
  // transient ~50ms USB write timeout worth retrying (confirmed via packet
  // capture - usually clears on the very next attempt) or, for some content,
  // persistent - retrying that forever with no cap collapsed the tick rate
  // from thousands/sec to ~20/sec in practice. kMaxSendFrameRetries bounds
  // the damage: give up and fall back to the old abandon-and-wait-for-new-
  // content behavior after a few failed cycles in a row.
  static constexpr uint8_t kMaxSendFrameRetries = 3;
  uint8_t m_sendFrameFailCount[kMASMK1_nDisplays]{};
  tRawData m_leds;
  tRawData m_buttons;

  std::bitset<kMASMK1_nButtons> m_buttonStates;
  unsigned m_encoderValues[kMASMK1_nEncoders];

  unsigned m_padsData[kMASMK1_nPads];
  std::bitset<kMASMK1_nPads> m_padsStatus;

  bool m_isDirtyLedGroup0{true};
  bool m_isDirtyLedGroup1{true};
  bool m_encodersInitialized{false};

  TickStep m_tickStep{TickStep::SendFrame};

  // Physical MIDI IN/OUT DIN ports, bridged to virtual ALSA/CoreMIDI ports so
  // external MIDI gear connected to the MK1 shows up like any other MIDI
  // device. Independent of Device::sendMidiMsg()'s direct callers - this is
  // specifically about the hardware's own MIDI jacks.
  //
  // m_midiOutQueue is lock-free rather than a plain std::deque because it's
  // now genuinely cross-thread: sendMidiMsg() (the producer) can be called
  // from whatever thread the client code runs on, while writeMidiMsg() (the
  // consumer) runs on the dedicated MIDI thread started in init().
  LockFreeQueue<tRawData, kMASMK1_midiOutQueueCapacity> m_midiOutQueue;
  std::deque<uint8_t> m_midiInBuffer;
  std::unique_ptr<RtMidiOut> m_pVirtualMidiIn;  // relays hardware MIDI IN outward
  std::unique_ptr<RtMidiIn> m_pVirtualMidiOut;  // receives from apps, sent to hardware MIDI OUT
};

//--------------------------------------------------------------------------------------------------

M_REGISTER_DEVICE_CLASS(
  MaschineMK1, "Maschine Controller", DeviceDescriptor::Type::USB, 0x17CC, 0x0808);

//--------------------------------------------------------------------------------------------------

} // namespace cabl
} // namespace sl

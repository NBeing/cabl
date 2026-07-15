/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "ButtonDebug.h"

#include <iostream>

using namespace sl;
using namespace sl::cabl;

namespace
{

// Buttons the Maschine MK1 actually reports, per MaschineMK1::deviceButton()
// (cabl/src/devices/ni/MaschineMK1.cpp). The rest of Device::Button is shared
// with other NI controllers and never fires on this hardware.
const char* buttonName(Device::Button button_)
{
#define M_NAME_CASE(idBtn)  \
  case Device::Button::idBtn: \
    return #idBtn

  switch (button_)
  {
    M_NAME_CASE(Mute);
    M_NAME_CASE(Solo);
    M_NAME_CASE(Select);
    M_NAME_CASE(Duplicate);
    M_NAME_CASE(Navigate);
    M_NAME_CASE(Keyboard);
    M_NAME_CASE(Pattern);
    M_NAME_CASE(Scene);
    M_NAME_CASE(Rec);
    M_NAME_CASE(Erase);
    M_NAME_CASE(Shift);
    M_NAME_CASE(Grid);
    M_NAME_CASE(TransportRight);
    M_NAME_CASE(TransportLeft);
    M_NAME_CASE(Loop);
    M_NAME_CASE(GroupA);
    M_NAME_CASE(GroupB);
    M_NAME_CASE(GroupC);
    M_NAME_CASE(GroupD);
    M_NAME_CASE(GroupE);
    M_NAME_CASE(GroupF);
    M_NAME_CASE(GroupG);
    M_NAME_CASE(GroupH);
    M_NAME_CASE(Control);
    M_NAME_CASE(Browse);
    M_NAME_CASE(BrowseLeft);
    M_NAME_CASE(Snap);
    M_NAME_CASE(AutoWrite);
    M_NAME_CASE(BrowseRight);
    M_NAME_CASE(Sampling);
    M_NAME_CASE(Step);
    M_NAME_CASE(DisplayButton1);
    M_NAME_CASE(DisplayButton2);
    M_NAME_CASE(DisplayButton3);
    M_NAME_CASE(DisplayButton4);
    M_NAME_CASE(DisplayButton5);
    M_NAME_CASE(DisplayButton6);
    M_NAME_CASE(DisplayButton7);
    M_NAME_CASE(DisplayButton8);
    M_NAME_CASE(NoteRepeat);
    M_NAME_CASE(Play);
    default:
      return "Unknown";
  }

#undef M_NAME_CASE
}

const auto kDoubleTapWindow = std::chrono::milliseconds(600);
const auto kScreensaverStep = std::chrono::milliseconds(1000);

// Every button the MK1 can actually light, per MaschineMK1::led(Device::Button)
// (MaschineMK1.cpp:565-622). Not the same set as buttonName()'s list above -
// a button can be pressable without having an LED, though on MK1 the two
// lists happen to be nearly identical.
const Device::Button kLightableButtons[] = {
  Device::Button::Mute,
  Device::Button::Solo,
  Device::Button::Select,
  Device::Button::Duplicate,
  Device::Button::Navigate,
  Device::Button::Keyboard,
  Device::Button::Pattern,
  Device::Button::Scene,
  Device::Button::Shift,
  Device::Button::Erase,
  Device::Button::Grid,
  Device::Button::TransportRight,
  Device::Button::Rec,
  Device::Button::Play,
  Device::Button::TransportLeft,
  Device::Button::Loop,
  Device::Button::GroupA,
  Device::Button::GroupB,
  Device::Button::GroupC,
  Device::Button::GroupD,
  Device::Button::GroupE,
  Device::Button::GroupF,
  Device::Button::GroupG,
  Device::Button::GroupH,
  Device::Button::AutoWrite,
  Device::Button::Snap,
  Device::Button::BrowseRight,
  Device::Button::BrowseLeft,
  Device::Button::Sampling,
  Device::Button::Browse,
  Device::Button::Step,
  Device::Button::Control,
  Device::Button::DisplayButton1,
  Device::Button::DisplayButton2,
  Device::Button::DisplayButton3,
  Device::Button::DisplayButton4,
  Device::Button::DisplayButton5,
  Device::Button::DisplayButton6,
  Device::Button::DisplayButton7,
  Device::Button::DisplayButton8,
  Device::Button::NoteRepeat,
};

} // namespace

//--------------------------------------------------------------------------------------------------

ButtonDebug::ButtonDebug() : m_rng(std::random_device{}())
{
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::initDevice()
{
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::logEvent(const std::string& line_)
{
  std::cout << line_ << std::endl;
  m_lastEvent = line_;
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_)
{
  if (button_ == Device::Button::DisplayButton1)
  {
    m_allLit = buttonState_;
    logEvent(
      std::string("button  DisplayButton1 = ") + (buttonState_ ? "1  [hold: light everything]" : "0"));
    requestDeviceUpdate();
    return;
  }

  if (button_ == Device::Button::DisplayButton2)
  {
    if (buttonState_)
    {
      m_mode = (m_mode == Mode::Ramp) ? Mode::Screensaver : Mode::Ramp;
      if (m_mode == Mode::Screensaver)
      {
        m_screensaverPhase = 0;
        m_lastScreensaverStep = std::chrono::steady_clock::now();
      }
    }
    logEvent(std::string("button  DisplayButton2 = ") + (buttonState_ ? "1" : "0")
      + "  [screensaver " + (m_mode == Mode::Screensaver ? "ON]" : "OFF]"));
    requestDeviceUpdate();
    return;
  }

  std::string line = std::string("button  ") + buttonName(button_) + " = "
    + (buttonState_ ? "1" : "0") + (shiftState_ ? "  [shift]" : "");

  if (!buttonState_)
  {
    logEvent(line);
    return;
  }

  auto key = static_cast<uint8_t>(button_);
  auto now = std::chrono::steady_clock::now();

  auto it = m_lastPress.find(key);
  if (it != m_lastPress.end())
  {
    auto deltaMs
      = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    line += "  (delta " + std::to_string(deltaMs) + "ms)";

    if (now - it->second <= kDoubleTapWindow)
    {
      double ratio = m_brightnessRatio(m_rng);
      m_buttonBrightness[key] = ratio;
      line += "  -> DOUBLE-TAP! brightness = " + std::to_string(static_cast<int>(ratio * 100)) + "%";
      m_lastPress.erase(it);
      logEvent(line);
      return;
    }
  }

  m_lastPress[key] = now;
  logEvent(line);
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::keyChanged(unsigned index_, double value_, bool shiftPressed_)
{
  logEvent("pad     #" + std::to_string(index_) + " = " + std::to_string(value_)
    + (shiftPressed_ ? "  [shift]" : ""));
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_)
{
  logEvent("encoder #" + std::to_string(encoder_) + " " + (valueIncreased_ ? "++" : "--")
    + (shiftPressed_ ? "  [shift]" : ""));
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::controlChanged(unsigned pot_, double value_, bool shiftPressed_)
{
  logEvent("pot     #" + std::to_string(pot_) + " = " + std::to_string(value_)
    + (shiftPressed_ ? "  [shift]" : ""));
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::renderPads(unsigned phase_)
{
  for (unsigned pad = 0; pad < mk1led::kNumPads; pad++)
  {
    unsigned shifted = (pad + phase_) % mk1led::kNumPads;
    double ratio = static_cast<double>(shifted) / (mk1led::kNumPads - 1);
    device()->setKeyLed(pad, mk1led::brightness(ratio));
  }
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::renderAllLit()
{
  for (unsigned pad = 0; pad < mk1led::kNumPads; pad++)
  {
    device()->setKeyLed(pad, mk1led::full());
  }
  for (auto button : kLightableButtons)
  {
    device()->setButtonLed(button, mk1led::full());
  }
}

//--------------------------------------------------------------------------------------------------

void ButtonDebug::render()
{
  if (m_allLit)
  {
    renderAllLit();
  }
  else
  {
    if (m_mode == Mode::Screensaver)
    {
      auto now = std::chrono::steady_clock::now();
      if (now - m_lastScreensaverStep >= kScreensaverStep)
      {
        m_screensaverPhase = (m_screensaverPhase + 1) % mk1led::kNumPads;
        m_lastScreensaverStep = now;
      }

      // Print/light using the same (post-advance) value, so the message
      // always matches what's actually lit - no off-by-one between them.
      std::string line = "screensaver: pad index " + std::to_string(m_screensaverPhase)
        + " is lit -- what pad number do you see?";
      if (line != m_lastEvent)
      {
        std::cout << line << std::endl;
        m_lastEvent = line;
      }

      for (unsigned pad = 0; pad < mk1led::kNumPads; pad++)
      {
        device()->setKeyLed(pad, pad == m_screensaverPhase ? mk1led::full() : mk1led::off());
      }
      requestDeviceUpdate(); // keep animating
    }
    else
    {
      renderPads(0);
    }

    // Buttons hold whatever brightness the double-tap feature last set (off if
    // never touched) - this is the persisted state the "light everything"
    // override above temporarily replaces and this restores on release.
    for (auto button : kLightableButtons)
    {
      auto it = m_buttonBrightness.find(static_cast<uint8_t>(button));
      device()->setButtonLed(
        button, it != m_buttonBrightness.end() ? mk1led::brightness(it->second) : mk1led::off());
    }
  }

  device()->graphicDisplay(0)->black();
  device()->graphicDisplay(0)->putText(4, 24, m_lastEvent.c_str(), {0xff});
}

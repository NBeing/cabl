/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/cabl.h>

#include "MenuUi.h"

namespace sl
{

using namespace cabl;

// Real-MK1-hardware backend for MenuUi. Turn the encoder to move focus
// between items, Select to activate the focused item, Erase to jump focus
// back to the first item. See TerminalMenu.h for the hardware-free
// terminal simulator that shares the same menu logic via MenuUi.
class LvglMenu : public Client, private MenuUi
{
public:
  void initDevice() override;
  void render() override;

  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_) override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;

protected:
  void setPixel(uint16_t x_, uint16_t y_, uint8_t brightness_) override;

private:
  // Encoder index that drives menu navigation - confirmed on hardware; MK1
  // has 11 encoders total.
  static constexpr unsigned kMenuEncoderIndex = 1;

  // The MK1 encoder fires more than one raw tick per physical detent, so a
  // 1:1 mapping made the menu feel twitchy - require a few raw ticks per
  // menu step instead. Raise/lower to tune feel.
  static constexpr int kRawTicksPerStep = 3;
  int m_rawTicks{0};
};

} // namespace sl

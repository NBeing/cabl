/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "LvglMenu.h"

#include <cstdlib>
#include <iostream>

namespace sl
{

//--------------------------------------------------------------------------------------------------

void LvglMenu::initDevice()
{
  MenuUi::init(device()->graphicDisplay(0)->width() * device()->numOfGraphicDisplays(),
    device()->graphicDisplay(0)->height());

  std::cout << "lvgl-menu ready. Turn encoder #" << kMenuEncoderIndex
            << " to move focus, Select to activate, Erase to jump to the first item."
            << std::endl;
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::render()
{
  MenuUi::render();
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::setPixel(uint16_t x_, uint16_t y_, uint8_t brightness_)
{
  uint16_t dispWidth = device()->graphicDisplay(0)->width();
  uint8_t dispNum = static_cast<uint8_t>(x_ / dispWidth);
  uint16_t localX = static_cast<uint16_t>(x_ - dispWidth * dispNum);
  device()->graphicDisplay(dispNum)->setPixel(localX, y_, brightness_);
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::buttonChanged(Device::Button button_, bool buttonState_, bool)
{
  if (button_ == Device::Button::Select)
  {
    onSelectChanged(buttonState_);
  }
  else if (button_ == Device::Button::Erase && buttonState_)
  {
    onErasePressed();
  }
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::encoderChanged(unsigned encoder_, bool valueIncreased_, bool)
{
  if (encoder_ == kMenuEncoderIndex)
  {
    m_rawTicks += valueIncreased_ ? 1 : -1;
    if (std::abs(m_rawTicks) >= kRawTicksPerStep)
    {
      onEncoderChanged(m_rawTicks > 0);
      m_rawTicks = 0;
    }
  }
  requestDeviceUpdate();
}

} // namespace sl

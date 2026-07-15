/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "KnobDisplay.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace
{
const sl::Color kColor_Green{0, 0xff, 0, 0x77};
const sl::Color kColor_White{0xff, 0xff, 0xff, 0xff};
const sl::Color kColor_Black{0, 0, 0, 0};
} // namespace

namespace sl
{

//--------------------------------------------------------------------------------------------------

void KnobDisplay::initDevice()
{
  std::cout << "[KnobDisplay] Device initialized!" << std::endl;
  std::cout << "[KnobDisplay] Number of displays: " << device()->numOfGraphicDisplays() << std::endl;
  
  // Initialize displays
  for(unsigned i = 0; i < device()->numOfGraphicDisplays() && i < 2; i++)
  {
    if(device()->graphicDisplay(i))
    {
      std::cout << "[KnobDisplay] Display " << i << " size: " 
                << device()->graphicDisplay(i)->width() << "x" 
                << device()->graphicDisplay(i)->height() << std::endl;
      drawDisplay(i);
    }
  }
  
  // Initialize all pad LEDs to dim state
  for(unsigned i = 0; i < 8; i++)
  {
    device()->setKeyLed(i, kColor_Black);
  }
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::render()
{
  // Render is called regularly - we update displays here
  static unsigned renderCount = 0;
  renderCount++;
  
  // Update pad decay every 5 render cycles
  if(renderCount % 5 == 0)
  {
    updatePadDecay();
  }
  
  // Update displays every 2 render cycles for smooth animation
  if(renderCount % 2 == 0)
  {
    for(unsigned i = 0; i < device()->numOfGraphicDisplays() && i < 2; i++)
    {
      drawDisplay(i);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_)
{
  std::cout << "[KnobDisplay] Button " << static_cast<int>(button_) 
            << (buttonState_ ? " pressed" : " released") << std::endl;
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_)
{
  // Encoders are ignored in pad display mode
  std::cout << "[KnobDisplay] Encoder " << encoder_ << " ignored (pad mode)" << std::endl;
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::keyChanged(unsigned index_, double value_, bool shiftPressed)
{
  // Only track first 8 pads (0-7)
  if(index_ < 8)
  {
    // Convert velocity (0.0-1.0) to display value (0-127)
    uint8_t displayValue = static_cast<uint8_t>(value_ * 127);
    m_padValues[index_] = displayValue;
    m_padDecayCounters[index_] = 0; // Reset decay counter
    
    std::cout << "[KnobDisplay] Pad " << (index_ + 1) << " hit with velocity " 
              << displayValue << " (" << value_ << ")" << std::endl;
    
    // Light up the corresponding pad LED based on hit strength
    uint8_t ledBrightness = static_cast<uint8_t>(value_ * 255);
    device()->setKeyLed(index_, {ledBrightness, 0, 0}); // Red color intensity
    
    // Determine which display to update (pads 0-3 on display 0, pads 4-7 on display 1)
    unsigned displayIndex = (index_ < 4) ? 0 : 1;
    drawDisplay(displayIndex);
  }
  else
  {
    std::cout << "[KnobDisplay] Pad " << (index_ + 1) << " ignored (only tracking first 8)" << std::endl;
  }
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::controlChanged(unsigned pot_, double value_, bool shiftPressed)
{
  std::cout << "[KnobDisplay] Control " << pot_ << " = " << value_ << std::endl;
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::drawPadValue(unsigned displayIndex_, unsigned padIndex_, uint8_t value_)
{
  auto display = device()->graphicDisplay(displayIndex_);
  if(!display) return;
  
  unsigned displayWidth = display->width();
  unsigned displayHeight = display->height();
  
  // Calculate position for this pad on the display
  unsigned padWidth = displayWidth / 4;  // 4 pads per display
  unsigned localPadIndex = padIndex_ % 4;  // 0-3 for each display
  unsigned x = localPadIndex * padWidth;
  
  // Clear the pad area
  display->rectangleFilled(x, 0, padWidth, displayHeight, kColor_Black, kColor_Black);
  
  // Draw bar graph
  unsigned barHeight = (value_ * (displayHeight - 16)) / 127;  // Leave space for text
  unsigned barWidth = padWidth - 4;  // Some padding
  unsigned barX = x + 2;
  unsigned barY = displayHeight - barHeight - 12;  // Leave space for text at bottom
  
  // Color intensity based on value (red gradient)
  uint8_t intensity = static_cast<uint8_t>((value_ * 255) / 127);
  sl::Color barColor{intensity, static_cast<uint8_t>(intensity/4), 0, 255}; // Red-orange gradient
  
  // Draw the bar (filled) with intensity-based color
  display->rectangleFilled(barX, barY, barWidth, barHeight, barColor, barColor);
  
  // Draw border around bar area
  display->rectangle(barX - 1, displayHeight - (displayHeight - 16) - 12, 
                    barWidth + 2, displayHeight - 16 + 1, kColor_White);
  
  // Draw value text at bottom
  std::stringstream ss;
  ss << static_cast<int>(value_);
  std::string valueStr = ss.str();
  
  // Calculate text position (centered in pad area)
  unsigned textX = x + (padWidth / 2) - (valueStr.length() * 3); // Rough centering
  unsigned textY = displayHeight - 10;
  
  // Use white text for visibility
  display->putText(textX, textY, valueStr.c_str(), {0xff});
  
  // Draw pad label at top
  std::stringstream labelSs;
  labelSs << "P" << (padIndex_ + 1); // P1, P2, etc.
  std::string labelStr = labelSs.str();
  unsigned labelX = x + (padWidth / 2) - (labelStr.length() * 3); // Rough centering
  
  // Use white text for label
  display->putText(labelX, 4, labelStr.c_str(), {0xff});
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::drawDisplay(unsigned displayIndex_)
{
  auto display = device()->graphicDisplay(displayIndex_);
  if(!display) return;
  
  // Clear display
  display->black();
  
  // Draw pads for this display
  unsigned startPad = displayIndex_ * 4;
  for(unsigned i = 0; i < 4; i++)
  {
    unsigned padIndex = startPad + i;
    if(padIndex < 8)
    {
      drawPadValue(displayIndex_, padIndex, m_padValues[padIndex]);
    }
  }
}

//--------------------------------------------------------------------------------------------------

void KnobDisplay::updatePadDecay()
{
  for(unsigned i = 0; i < 8; i++)
  {
    if(m_padValues[i] > 0)
    {
      m_padDecayCounters[i]++;
      
      // Start decay after 10 cycles (about half a second)
      if(m_padDecayCounters[i] > 10)
      {
        // Decay by 2 points per cycle for smooth animation
        if(m_padValues[i] >= 2)
        {
          m_padValues[i] -= 2;
        }
        else
        {
          m_padValues[i] = 0;
        }
        
        // Also decay the LED
        if(m_padValues[i] > 0)
        {
          uint8_t ledBrightness = static_cast<uint8_t>((m_padValues[i] * 255) / 127);
          device()->setKeyLed(i, {ledBrightness, 0, 0});
        }
        else
        {
          device()->setKeyLed(i, kColor_Black);
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

} // namespace sl

/*
        ##########    Copy  void keyChanged(unsigned index_, double value_, bool shiftPressed) override;
  void controlChanged(unsigned pot_, double value_, bool shiftPressed) override;

private:
  void drawPadValue(unsigned displayIndex_, unsigned padIndex_, uint8_t value_);
  void drawDisplay(unsigned displayIndex_);
  void updatePadDecay();
  
  // Store pad values (0-127) for first 8 pads
  std::array<uint8_t, 8> m_padValues = {{0}};
  // Track time since last hit for decay effect
  std::array<unsigned, 8> m_padDecayCounters = {{0}}; 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <future>
#include <cstdint>
#include <array>

#include <cabl/cabl.h>

namespace sl
{

using namespace cabl;

class KnobDisplay : public Client
{
public:

  void initDevice() override;
  void render() override;

  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_) override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;
  void keyChanged(unsigned index_, double value_, bool shiftPressed) override;
  void controlChanged(unsigned pot_, double value_, bool shiftPressed) override;

private:
  void drawPadValue(unsigned displayIndex_, unsigned padIndex_, uint8_t value_);
  void drawDisplay(unsigned displayIndex_);
  
  // Store pad LED values (0-127)
  std::array<uint8_t, 8> m_padValues = {{0}};
};

} // namespace sl

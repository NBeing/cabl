/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/cabl.h>

#include "Mk1Led.h"

#include <chrono>
#include <random>
#include <string>
#include <unordered_map>

namespace sl
{

using namespace cabl;

class ButtonDebug : public Client
{
public:
  ButtonDebug();

  void initDevice() override;
  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_) override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;
  void keyChanged(unsigned index_, double value_, bool shiftPressed_) override;
  void controlChanged(unsigned pot_, double value_, bool shiftPressed_) override;
  void render() override;

private:
  enum class Mode
  {
    Ramp,
    Screensaver,
  };

  void logEvent(const std::string& line_);
  void renderPads(unsigned phase_);
  void renderAllLit();

  std::string m_lastEvent;
  std::unordered_map<uint8_t, std::chrono::steady_clock::time_point> m_lastPress;
  std::unordered_map<uint8_t, double> m_buttonBrightness; // button ordinal -> ratio [0.0, 1.0]
  std::mt19937 m_rng;
  std::uniform_real_distribution<double> m_brightnessRatio{0.15, 1.0};

  Mode m_mode = Mode::Ramp;
  bool m_allLit = false;
  unsigned m_screensaverPhase = 0;
  std::chrono::steady_clock::time_point m_lastScreensaverStep;
};

} // namespace sl

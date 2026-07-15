/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/cabl.h>

#include <chrono>
#include <string>

namespace sl
{

using namespace cabl;

// Cycles through every public Canvas (and relevant Color) method, a few
// seconds each, on both of the MK1's graphic displays. Not a "how to build a
// UI" example - a coverage sweep of the drawing API surface.
class ScreenTest : public Client
{
public:
  enum class Demo
  {
    FillModes,
    Invert,
    FillPattern,
    PixelAccess,
    Lines,
    Rectangles,
    RoundedRectangles,
    Triangles,
    Circles,
    Bitmap,
    CanvasCopy,
    Characters,
    Fonts,
    ColorApi,
    Count, // sentinel - not a real demo
  };

  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_) override;
  void render() override;

private:
  void advance();
  void showDemo();

  Demo m_demo{Demo::FillModes};
  bool m_started{false};
  std::chrono::steady_clock::time_point m_lastAdvance;
};

} // namespace sl

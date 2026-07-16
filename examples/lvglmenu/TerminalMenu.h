/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "MenuUi.h"

#include <cstdint>
#include <termios.h>
#include <vector>

namespace sl
{

// Hardware-free terminal backend for MenuUi: renders the simulated MK1
// display as Unicode Braille art and reads keyboard input in raw terminal
// mode, standing in for the encoder/Select/Erase buttons. No cabl/USB
// dependency at all - for iterating on menu logic without the real device
// attached. See LvglMenu.h for the real-hardware backend.
class TerminalMenu : private MenuUi
{
public:
  // Matches the real MK1's two 255x64 displays side by side, so the
  // simulated menu behaves the same as the real thing.
  static constexpr uint16_t kWidth = 510;
  static constexpr uint16_t kHeight = 64;

  TerminalMenu();

  // Runs until 'q' is pressed. Always restores terminal settings before
  // returning, including on early exit.
  void run();

protected:
  void setPixel(uint16_t x_, uint16_t y_, uint8_t brightness_) override;

private:
  void enableRawMode();
  void restoreTerminalMode();
  void pollInput();
  void redraw();

  std::vector<uint8_t> m_framebuffer;
  bool m_running{true};
  unsigned m_linesDrawn{0};
  termios m_savedTermios{};
  int m_savedFdFlags{0};

  // LVGL's encoder indev polls on its own ~50ms cadence (LV_INDEV_DEF_READ_PERIOD),
  // independent of our render loop - a same-instant press+release on Enter could
  // land between two polls and never be observed. Holding "pressed" for a few
  // frames guarantees at least one poll sees it.
  static constexpr int kSelectHoldFrames = 3;
  int m_selectHoldFrames{0};
};

} // namespace sl

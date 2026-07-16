/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "TerminalMenu.h"

#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

namespace sl
{

namespace
{

// Packs a 2-wide x 4-tall block of monochrome pixels into one Unicode
// Braille Pattern codepoint (0x2800 + dot-mask), giving a 2x horizontal /
// 4x vertical size reduction versus one character per pixel - the 510px
// MK1 display becomes ~255 terminal columns instead of 510. Standard
// braille dot layout within the block:
//   1 4
//   2 5
//   3 6
//   7 8
constexpr uint8_t kDotBit[4][2] = {
  {0x01, 0x08},
  {0x02, 0x10},
  {0x04, 0x20},
  {0x40, 0x80},
};

// Braille codepoints are always U+2800-U+28FF, comfortably inside the
// 3-byte UTF-8 range - standard 3-byte encoding of the codepoint.
void appendBrailleChar(std::string& out_, uint8_t dotMask_)
{
  const uint16_t codepoint = 0x2800 + dotMask_;
  out_.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
  out_.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
  out_.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
}

} // namespace

//--------------------------------------------------------------------------------------------------

TerminalMenu::TerminalMenu() : m_framebuffer(static_cast<size_t>(kWidth) * kHeight, 0)
{
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::setPixel(uint16_t x_, uint16_t y_, uint8_t brightness_)
{
  if (x_ < kWidth && y_ < kHeight)
  {
    m_framebuffer[static_cast<size_t>(y_) * kWidth + x_] = (brightness_ >= 128) ? 1 : 0;
  }
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::enableRawMode()
{
  tcgetattr(STDIN_FILENO, &m_savedTermios);
  termios raw = m_savedTermios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  // VMIN/VTIME above only govern real TTY line-discipline behavior - if
  // stdin is ever a pipe or redirected file instead, read() would still
  // block waiting for more data without this. Setting O_NONBLOCK on the fd
  // itself makes the non-blocking behavior unconditional.
  m_savedFdFlags = fcntl(STDIN_FILENO, F_GETFL);
  fcntl(STDIN_FILENO, F_SETFL, m_savedFdFlags | O_NONBLOCK);
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::restoreTerminalMode()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &m_savedTermios);
  fcntl(STDIN_FILENO, F_SETFL, m_savedFdFlags);
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::pollInput()
{
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1)
  {
    switch (c)
    {
      case 'q':
        m_running = false;
        break;
      case 'j':
        onEncoderChanged(false);
        break;
      case 'k':
        onEncoderChanged(true);
        break;
      case '\n':
      case '\r':
        onSelectChanged(true);
        m_selectHoldFrames = kSelectHoldFrames;
        break;
      case 'e':
      case 0x7F: // Backspace
        onErasePressed();
        break;
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::redraw()
{
  // Cursor-up-and-reprint rather than a full clear each frame, same trick
  // as EventLoadHarness's dashboard - avoids flicker and stays readable
  // over a slow serial/SSH link.
  std::string out;
  if (m_linesDrawn > 0)
  {
    out += "\x1b[" + std::to_string(m_linesDrawn) + "A";
  }

  for (uint16_t blockY = 0; blockY < kHeight; blockY += 4)
  {
    for (uint16_t blockX = 0; blockX < kWidth; blockX += 2)
    {
      uint8_t dotMask = 0;
      for (int dy = 0; dy < 4; dy++)
      {
        for (int dx = 0; dx < 2; dx++)
        {
          uint16_t x = blockX + static_cast<uint16_t>(dx);
          uint16_t y = blockY + static_cast<uint16_t>(dy);
          if (m_framebuffer[static_cast<size_t>(y) * kWidth + x])
          {
            dotMask |= kDotBit[dy][dx];
          }
        }
      }
      appendBrailleChar(out, dotMask);
    }
    out += "\x1b[K\n"; // clear to end of line, in case the terminal was wider before
  }

  std::fwrite(out.data(), 1, out.size(), stdout);
  std::fflush(stdout);
  m_linesDrawn = kHeight / 4;
}

//--------------------------------------------------------------------------------------------------

void TerminalMenu::run()
{
  enableRawMode();
  MenuUi::init(kWidth, kHeight);

  std::cout << "lvgl-menu-sim ready. j/k to move focus, Enter to activate, "
               "e/Backspace to jump to the first item, q to quit."
            << std::endl;

  auto nextTick = std::chrono::steady_clock::now();
  const auto period = std::chrono::milliseconds(40);
  while (m_running)
  {
    pollInput();

    if (m_selectHoldFrames > 0 && --m_selectHoldFrames == 0)
    {
      onSelectChanged(false);
    }

    MenuUi::render();
    redraw();
    nextTick += period;
    std::this_thread::sleep_until(nextTick);
  }

  restoreTerminalMode();
  std::cout << std::endl;
}

} // namespace sl

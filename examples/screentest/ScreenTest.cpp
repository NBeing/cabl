/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "ScreenTest.h"

#include <iostream>

using namespace sl;
using namespace sl::cabl;

namespace
{
const auto kDemoStep = std::chrono::milliseconds(2500);
const Color kWhite(0xff);

// 8x8, 1 bit per pixel, MSB-first, row-major (per Canvas::putBitmap's own
// unpacking: pBitmap_[(i>>3) + j*(w_>>3)] & (0x01 << (7-(i&7)))).
const uint8_t kCheckerBitmap[8] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};

const char* demoName(ScreenTest::Demo demo_)
{
  switch (demo_)
  {
    case ScreenTest::Demo::FillModes:
      return "white() / black()";
    case ScreenTest::Demo::Invert:
      return "invert()";
    case ScreenTest::Demo::FillPattern:
      return "fill(uint8_t)";
    case ScreenTest::Demo::PixelAccess:
      return "setPixel() / pixel()";
    case ScreenTest::Demo::Lines:
      return "line() / lineHorizontal() / lineVertical()";
    case ScreenTest::Demo::Rectangles:
      return "rectangle() / rectangleFilled()";
    case ScreenTest::Demo::RoundedRectangles:
      return "rectangleRounded() / rectangleRoundedFilled()";
    case ScreenTest::Demo::Triangles:
      return "triangle() / triangleFilled()";
    case ScreenTest::Demo::Circles:
      return "circle() / circleFilled() - all 9 CircleTypes";
    case ScreenTest::Demo::Bitmap:
      return "putBitmap()";
    case ScreenTest::Demo::CanvasCopy:
      return "putCanvas()";
    case ScreenTest::Demo::Characters:
      return "putCharacter()";
    case ScreenTest::Demo::Fonts:
      return "putText() - normal/small/big fonts + spacing_";
    case ScreenTest::Demo::ColorApi:
      return "Color API + Canvas introspection (console only)";
    default:
      return "?";
  }
}

Canvas::CircleType kAllCircleTypes[] = {
  Canvas::CircleType::Full,
  Canvas::CircleType::SemiLeft,
  Canvas::CircleType::SemiTop,
  Canvas::CircleType::SemiRight,
  Canvas::CircleType::SemiBottom,
  Canvas::CircleType::QuarterTopLeft,
  Canvas::CircleType::QuarterTopRight,
  Canvas::CircleType::QuarterBottomRight,
  Canvas::CircleType::QuarterBottomLeft,
};

} // namespace

//--------------------------------------------------------------------------------------------------

void ScreenTest::buttonChanged(Device::Button, bool buttonState_, bool)
{
  if (buttonState_)
  {
    advance();
  }
}

//--------------------------------------------------------------------------------------------------

void ScreenTest::advance()
{
  m_demo = static_cast<Demo>((static_cast<int>(m_demo) + 1) % static_cast<int>(Demo::Count));
  m_lastAdvance = std::chrono::steady_clock::now();
  showDemo();
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void ScreenTest::showDemo()
{
  std::cout << "demo: " << demoName(m_demo) << std::endl;
}

//--------------------------------------------------------------------------------------------------

void ScreenTest::render()
{
  Canvas* d0 = device()->graphicDisplay(0);
  Canvas* d1 = device()->graphicDisplay(1);

  if (!m_started)
  {
    m_started = true;
    m_lastAdvance = std::chrono::steady_clock::now();
    showDemo();
  }
  else if (std::chrono::steady_clock::now() - m_lastAdvance >= kDemoStep)
  {
    advance();
  }

  d0->black();
  d1->black();

  switch (m_demo)
  {
    case Demo::FillModes:
    {
      d0->white();
      d1->black();
      break;
    }

    case Demo::Invert:
    {
      d0->black();
      d0->rectangleFilled(40, 10, 80, 40, kWhite, kWhite);
      d0->invert(); // black bg -> white, white rect -> black
      break;
    }

    case Demo::FillPattern:
    {
      d0->fill(0xAA); // 10101010 repeated - vertical stripes
      d1->fill(0x0F); // 00001111 repeated - different stripe phase
      break;
    }

    case Demo::PixelAccess:
    {
      for (unsigned i = 0; i < d0->width() && i < d0->height() * 4; i += 2)
      {
        d0->setPixel(i, i / 4, kWhite);
      }
      Color readBack = d0->pixel(0, 0);
      std::cout << "  pixel(0,0) after setPixel -> mono=" << static_cast<int>(readBack.mono())
                << std::endl;
      break;
    }

    case Demo::Lines:
    {
      d0->line(0, 0, d0->width() - 1, d0->height() - 1, kWhite);
      d0->line(0, d0->height() - 1, d0->width() - 1, 0, kWhite);
      d0->lineHorizontal(0, d0->height() / 2, d0->width(), kWhite);
      d0->lineVertical(d0->width() / 2, 0, d0->height(), kWhite);
      break;
    }

    case Demo::Rectangles:
    {
      d0->rectangle(4, 4, 100, 50, kWhite);
      d1->rectangleFilled(4, 4, 100, 50, kWhite, kWhite);
      break;
    }

    case Demo::RoundedRectangles:
    {
      d0->rectangleRounded(4, 4, 100, 50, 12, kWhite);
      d1->rectangleRoundedFilled(4, 4, 100, 50, 12, kWhite, kWhite);
      break;
    }

    case Demo::Triangles:
    {
      d0->triangle(10, 55, 60, 5, 110, 55, kWhite);
      d1->triangleFilled(10, 55, 60, 5, 110, 55, kWhite, kWhite);
      break;
    }

    case Demo::Circles:
    {
      unsigned x = 15;
      for (auto type : kAllCircleTypes)
      {
        d0->circle(x, 32, 14, kWhite, type);
        d1->circleFilled(x, 32, 14, kWhite, kWhite, type);
        x += 28;
      }
      break;
    }

    case Demo::Bitmap:
    {
      for (unsigned y = 0; y < d0->height(); y += 8)
      {
        for (unsigned x = 0; x < d0->width(); x += 8)
        {
          d0->putBitmap(x, y, 8, 8, kCheckerBitmap, kWhite);
        }
      }
      break;
    }

    case Demo::CanvasCopy:
    {
      d1->black();
      d1->circleFilled(30, 32, 25, kWhite, kWhite);
      d1->rectangle(0, 0, d1->width() - 1, d1->height() - 1, kWhite);

      d0->black();
      d0->putCanvas(*d1, 60, 0, 0, 0, 60, d1->height()); // copy a 60px-wide slice
      break;
    }

    case Demo::Characters:
    {
      unsigned x = 4;
      for (char c : std::string("HI MK1"))
      {
        d0->putCharacter(x, 20, c, kWhite);
        x += 14;
      }
      break;
    }

    case Demo::Fonts:
    {
      d0->putText(4, 4, "normal font", kWhite, "normal");
      d0->putText(4, 24, "small font", kWhite, "small");
      d0->putText(4, 44, "big", kWhite, "big");
      d1->putText(4, 4, "spacing=0", kWhite, "normal", 0);
      d1->putText(4, 24, "spacing=4", kWhite, "normal", 4);
      break;
    }

    case Demo::ColorApi:
    {
      Color mono(0x80);
      Color rgb(255, 0, 0);
      Color rgbMono(255, 0, 0, 0x40);
      Color inverted(BlendMode::Invert);

      std::cout << "  Color(0x80).mono()=" << static_cast<int>(mono.mono())
                << " active()=" << mono.active() << std::endl;
      std::cout << "  Color(255,0,0): r=" << static_cast<int>(rgb.red())
                << " g=" << static_cast<int>(rgb.green()) << " b=" << static_cast<int>(rgb.blue())
                << " mono()=" << static_cast<int>(rgb.mono()) << " (max-decomposed)" << std::endl;
      std::cout << "  Color(255,0,0,0x40).mono()=" << static_cast<int>(rgbMono.mono())
                << " (explicit override)" << std::endl;
      std::cout << "  Color(BlendMode::Invert).mono()=" << static_cast<int>(inverted.mono())
                << " transparent()=" << inverted.transparent() << std::endl;
      std::cout << "  mono.distance(rgb)=" << mono.distance(rgb)
                << " getValue()=" << mono.getValue() << std::endl;
      std::cout << "  display0: width()=" << d0->width() << " height()=" << d0->height()
                << " canvasWidthInBytes()=" << d0->canvasWidthInBytes()
                << " numberOfChunks()=" << d0->numberOfChunks()
                << " bufferSize()=" << d0->bufferSize() << std::endl;

      d0->rectangleFilled(20, 10, 60, 40, kWhite, kWhite);
      break;
    }

    default:
      break;
  }

  requestDeviceUpdate(); // keep the auto-advance timer ticking
}

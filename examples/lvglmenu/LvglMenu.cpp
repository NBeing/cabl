/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "LvglMenu.h"

#include <iostream>
#include <sys/time.h>

namespace
{
// Encoder index that drives menu navigation. MK1 has 11 encoders; which
// physical knob this is hasn't been confirmed on hardware yet - console logs
// the raw index on every turn so it's a one-line change once we know.
const unsigned kMenuEncoderIndex = 1; // confirmed on hardware

const unsigned kDisplayBufSize = 128 * 1024;

sl::cabl::Coordinator::tDevicePtr g_pDevice;
sl::LvglMenu* g_pMenu = nullptr;

extern "C" void fbdev_flush(lv_disp_drv_t* drv_, const lv_area_t* area_, lv_color_t* colorP_)
{
  uint16_t dispWidth = g_pDevice->graphicDisplay(0)->width();

  for (int32_t y = area_->y1; y <= area_->y2; y++)
  {
    for (int32_t x = area_->x1; x <= area_->x2; x++)
    {
      uint8_t dispNum = x / dispWidth;
      uint8_t brightness = colorP_->full; // LV_COLOR_DEPTH 8, theme kept strictly black/white
      unsigned localX = (x < dispWidth) ? x : (x - dispWidth * dispNum);
      g_pDevice->graphicDisplay(dispNum)->setPixel(localX, y, brightness);
      colorP_++;
    }
  }
  lv_disp_flush_ready(drv_);
}

extern "C" uint32_t custom_tick_get()
{
  static uint64_t startMs = 0;
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  uint64_t nowMs = (tv.tv_sec * 1000000ULL + tv.tv_usec) / 1000;
  if (startMs == 0)
  {
    startMs = nowMs;
  }
  return static_cast<uint32_t>(nowMs - startMs);
}

extern "C" void encoder_read(lv_indev_drv_t*, lv_indev_data_t* data_)
{
  data_->enc_diff = g_pMenu ? g_pMenu->takeEncDiff() : 0;
  data_->state = g_pMenu ? g_pMenu->selectState() : LV_INDEV_STATE_RELEASED;
}

} // namespace

namespace sl
{

//--------------------------------------------------------------------------------------------------

void LvglMenu::onItemClicked(lv_event_t* e_)
{
  lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e_));
  const char* text = lv_list_get_btn_text(lv_obj_get_parent(target), target);
  std::cout << "selected: " << (text ? text : "?") << std::endl;
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::buildMenu()
{
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

  lv_obj_t* list = lv_list_create(screen);
  lv_obj_set_size(list, device()->graphicDisplay(0)->width() * device()->numOfGraphicDisplays(),
    device()->graphicDisplay(0)->height());
  lv_obj_set_style_bg_color(list, lv_color_black(), 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 0, 0);

  const char* items[] = {"Oscillator", "Filter", "Envelope", "LFO", "About"};
  for (const char* item : items)
  {
    lv_obj_t* btn = lv_list_add_btn(list, nullptr, item);
    lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
    lv_obj_set_style_text_color(btn, lv_color_white(), 0);
    lv_obj_set_style_bg_color(btn, lv_color_white(), LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(btn, lv_color_black(), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btn, onItemClicked, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(m_pGroup, btn);

    if (m_pFirstItem == nullptr)
    {
      m_pFirstItem = btn;
    }
  }

  lv_group_focus_obj(m_pFirstItem);
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::initDevice()
{
  g_pDevice = device();
  g_pMenu = this;

  lv_init();

  static lv_color_t buf[kDisplayBufSize];
  static lv_disp_draw_buf_t dispBuf;
  lv_disp_draw_buf_init(&dispBuf, buf, nullptr, kDisplayBufSize);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.draw_buf = &dispBuf;
  dispDrv.flush_cb = fbdev_flush;
  dispDrv.hor_res = device()->graphicDisplay(0)->width() * device()->numOfGraphicDisplays();
  dispDrv.ver_res = device()->graphicDisplay(0)->height();
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t encDrv;
  lv_indev_drv_init(&encDrv);
  encDrv.type = LV_INDEV_TYPE_ENCODER;
  encDrv.read_cb = encoder_read;
  m_pEncoderIndev = lv_indev_drv_register(&encDrv);

  m_pGroup = lv_group_create();
  lv_indev_set_group(m_pEncoderIndev, m_pGroup);

  buildMenu();

  std::cout << "lvgl-menu ready. Turn encoder #" << kMenuEncoderIndex
            << " to move focus, Select to activate, Erase to jump to the first item."
            << std::endl;
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::render()
{
  lv_timer_handler();
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::buttonChanged(Device::Button button_, bool buttonState_, bool)
{
  if (button_ == Device::Button::Select)
  {
    m_selectState = buttonState_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  }
  else if (button_ == Device::Button::Erase && buttonState_ && m_pFirstItem)
  {
    lv_group_focus_obj(m_pFirstItem);
  }
  requestDeviceUpdate();
}

//--------------------------------------------------------------------------------------------------

void LvglMenu::encoderChanged(unsigned encoder_, bool valueIncreased_, bool)
{
  std::cout << "encoder #" << encoder_ << (valueIncreased_ ? " ++" : " --") << std::endl;
  if (encoder_ == kMenuEncoderIndex)
  {
    m_encDiff += valueIncreased_ ? 1 : -1;
  }
  requestDeviceUpdate();
}

} // namespace sl

/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "MenuUi.h"

#include <iostream>
#include <sys/time.h>

namespace
{

// Called by name (LV_TICK_CUSTOM_SYS_TIME_EXPR in lv_conf.h) from LVGL's own
// C code, not through a function pointer - has to stay a plain extern "C"
// free function rather than a MenuUi member.
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

} // namespace

namespace sl
{

//--------------------------------------------------------------------------------------------------

void MenuUi::sFlushCb(lv_disp_drv_t* drv_, const lv_area_t* area_, lv_color_t* colorP_)
{
  MenuUi* self = static_cast<MenuUi*>(drv_->user_data);

  for (int32_t y = area_->y1; y <= area_->y2; y++)
  {
    for (int32_t x = area_->x1; x <= area_->x2; x++)
    {
      // LV_COLOR_DEPTH 8, theme kept strictly black/white - .full is a plain
      // 0-255 intensity here, not a packed RGB value.
      self->setPixel(static_cast<uint16_t>(x), static_cast<uint16_t>(y), colorP_->full);
      colorP_++;
    }
  }
  lv_disp_flush_ready(drv_);
}

//--------------------------------------------------------------------------------------------------

void MenuUi::sEncoderReadCb(lv_indev_drv_t* drv_, lv_indev_data_t* data_)
{
  MenuUi* self = static_cast<MenuUi*>(drv_->user_data);
  data_->enc_diff = self->takeEncDiff();
  data_->state = self->selectState();
}

//--------------------------------------------------------------------------------------------------

int16_t MenuUi::takeEncDiff()
{
  int16_t diff = m_encDiff;
  m_encDiff = 0;
  return diff;
}

//--------------------------------------------------------------------------------------------------

lv_indev_state_t MenuUi::selectState() const
{
  return m_selectState;
}

//--------------------------------------------------------------------------------------------------

void MenuUi::onItemClicked(lv_event_t* e_)
{
  lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e_));
  const char* text = lv_list_get_btn_text(lv_obj_get_parent(target), target);
  std::cout << "selected: " << (text ? text : "?") << std::endl;
}

//--------------------------------------------------------------------------------------------------

void MenuUi::buildMenu()
{
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

  lv_obj_t* list = lv_list_create(screen);
  lv_obj_set_size(list, m_width, m_height);
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

void MenuUi::init(uint16_t width_, uint16_t height_)
{
  m_width = width_;
  m_height = height_;

  lv_init();

  m_displayBuf.resize(static_cast<size_t>(width_) * height_);
  lv_disp_draw_buf_init(&m_dispBuf, m_displayBuf.data(), nullptr, m_displayBuf.size());

  lv_disp_drv_init(&m_dispDrv);
  m_dispDrv.draw_buf = &m_dispBuf;
  m_dispDrv.flush_cb = &MenuUi::sFlushCb;
  m_dispDrv.hor_res = width_;
  m_dispDrv.ver_res = height_;
  m_dispDrv.user_data = this;
  lv_disp_drv_register(&m_dispDrv);

  lv_indev_drv_init(&m_encDrv);
  m_encDrv.type = LV_INDEV_TYPE_ENCODER;
  m_encDrv.read_cb = &MenuUi::sEncoderReadCb;
  m_encDrv.user_data = this;
  m_pEncoderIndev = lv_indev_drv_register(&m_encDrv);

  m_pGroup = lv_group_create();
  lv_indev_set_group(m_pEncoderIndev, m_pGroup);

  buildMenu();
}

//--------------------------------------------------------------------------------------------------

void MenuUi::render()
{
  lv_timer_handler();
}

//--------------------------------------------------------------------------------------------------

void MenuUi::onEncoderChanged(bool increased_)
{
  m_encDiff += increased_ ? 1 : -1;
}

//--------------------------------------------------------------------------------------------------

void MenuUi::onSelectChanged(bool pressed_)
{
  m_selectState = pressed_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

//--------------------------------------------------------------------------------------------------

void MenuUi::onErasePressed()
{
  if (m_pFirstItem)
  {
    lv_group_focus_obj(m_pFirstItem);
  }
}

//--------------------------------------------------------------------------------------------------

} // namespace sl

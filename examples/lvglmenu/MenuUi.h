/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include "lvgl.h"

#include <cstdint>
#include <vector>

namespace sl
{

// Backend-agnostic core of the encoder-navigated LVGL menu: LVGL init, the
// display/indev driver plumbing, and the menu content itself. A backend
// (real MK1 hardware - see LvglMenu.h - or the hardware-free terminal
// simulator - see TerminalMenu.h) supplies pixel output via setPixel() and
// feeds input through onEncoderChanged()/onSelectChanged()/onErasePressed().
// Everything here is identical regardless of where the pixels actually end
// up.
class MenuUi
{
public:
  virtual ~MenuUi() = default;

  void init(uint16_t width_, uint16_t height_);
  void render();

  // One already-debounced logical step per call - any hardware-specific
  // noise filtering (e.g. a real encoder firing several raw ticks per
  // physical detent) belongs in the backend, not here.
  void onEncoderChanged(bool increased_);
  void onSelectChanged(bool pressed_);
  void onErasePressed();

protected:
  virtual void setPixel(uint16_t x_, uint16_t y_, uint8_t brightness_) = 0;

private:
  void buildMenu();
  static void onItemClicked(lv_event_t* e_);

  static void sFlushCb(lv_disp_drv_t* drv_, const lv_area_t* area_, lv_color_t* colorP_);
  static void sEncoderReadCb(lv_indev_drv_t* drv_, lv_indev_data_t* data_);

  int16_t takeEncDiff();
  lv_indev_state_t selectState() const;

  std::vector<lv_color_t> m_displayBuf;
  lv_disp_draw_buf_t m_dispBuf{};
  lv_disp_drv_t m_dispDrv{};
  lv_indev_drv_t m_encDrv{};
  lv_indev_t* m_pEncoderIndev{nullptr};

  lv_group_t* m_pGroup{nullptr};
  lv_obj_t* m_pFirstItem{nullptr};

  int16_t m_encDiff{0};
  lv_indev_state_t m_selectState{LV_INDEV_STATE_RELEASED};

  uint16_t m_width{0};
  uint16_t m_height{0};
};

} // namespace sl

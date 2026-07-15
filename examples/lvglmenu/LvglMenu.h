/*
        ##########    Copyright (C) 2015 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#pragma once

#include <cabl/cabl.h>

#include "lvgl.h"

namespace sl
{

using namespace cabl;

// A real encoder-navigated menu built on LVGL - not a stock demo widget.
// Turn the encoder to move focus between items, Select to activate the
// focused item, Erase to jump focus back to the first item.
class LvglMenu : public Client
{
public:
  void initDevice() override;
  void render() override;

  void buttonChanged(Device::Button button_, bool buttonState_, bool shiftState_) override;
  void encoderChanged(unsigned encoder_, bool valueIncreased_, bool shiftPressed_) override;

  // Called by the LVGL encoder indev's read callback (a free function, not a
  // member) once per LVGL input poll.
  int16_t takeEncDiff()
  {
    int16_t diff = m_encDiff;
    m_encDiff = 0;
    return diff;
  }

  lv_indev_state_t selectState() const
  {
    return m_selectState;
  }

private:
  void buildMenu();
  static void onItemClicked(lv_event_t* e_);

  lv_group_t* m_pGroup{nullptr};
  lv_indev_t* m_pEncoderIndev{nullptr};
  lv_obj_t* m_pFirstItem{nullptr};

  int16_t m_encDiff{0};
  lv_indev_state_t m_selectState{LV_INDEV_STATE_RELEASED};
};

} // namespace sl

#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xpp/window.hpp>

#include "common.hpp"

POLYBAR_NS

class connection;

class window : public xpp::window<connection&> {
 public:
  using xpp::window<class connection&>::window;

  window& operator=(const xcb_window_t win);

  window create_checked(
      int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t mask = 0, const xcb_params_cw_t* p = nullptr);

  window change_event_mask(uint32_t mask);
  window ensure_event_mask(uint32_t event);

  window reconfigure_geom(uint16_t w, uint16_t h, int16_t x = 0, int16_t y = 0);
  window reconfigure_pos(int16_t x, int16_t y);
  window reconfigure_struts(uint16_t w, uint16_t h, int16_t x, bool bottom = false);

  void redraw();
};

POLYBAR_NS_END

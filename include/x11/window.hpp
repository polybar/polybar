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
      short int x, short int y, unsigned short int w, unsigned short int h, unsigned int mask = 0, const xcb_params_cw_t* p = nullptr);

  window change_event_mask(unsigned int mask);
  window ensure_event_mask(unsigned int event);

  window reconfigure_geom(unsigned short int w, unsigned short int h, short int x = 0, short int y = 0);
  window reconfigure_pos(short int x, short int y);
  window reconfigure_struts(unsigned short int w, unsigned short int h, short int x, bool bottom = false);

  void redraw();

  void visibility_notify(xcb_visibility_t state);
};

POLYBAR_NS_END

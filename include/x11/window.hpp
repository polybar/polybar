#pragma once

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include <xpp/window.hpp>

#include "common.hpp"

POLYBAR_NS

class connection;

class window : public xpp::window<connection&> {
 public:
  window(const window&) = default;
  using xpp::window<class connection&>::window;

  window reconfigure_geom(unsigned short int w, unsigned short int h, short int x = 0, short int y = 0);
  window reconfigure_pos(short int x, short int y);
  window reconfigure_struts(uint32_t w, uint32_t strut, uint32_t x, bool bottom = false);
};

POLYBAR_NS_END

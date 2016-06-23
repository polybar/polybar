#pragma once

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include <xpp.hpp>
#include <proto/randr.hpp>

namespace x
{
  typedef xpp::connection<xpp::randr::extension> connection;
  typedef xpp::event::registry<connection &, xpp::randr::extension> registry;

  typedef xpp::window<connection &> window;
  typedef xpp::window<xcb_connection_t *> xcb_window;

  typedef xpp::x::event::key_press<connection &> key_press;
  typedef xpp::x::event::key_release<connection &> key_release;
  typedef xpp::x::event::button_press<connection &> button_press;

  typedef xpp::randr::event::notify<connection &> randr_notify;
  typedef xpp::randr::event::screen_change_notify<connection &> randr_screen_change_notify;
}

namespace xcb
{
  typedef struct monitor_t monitor_t;
  struct monitor_t
  {
    char name[32] = "NONAME";
    xcb_rectangle_t bounds;
    int index = 0;
  };

  std::shared_ptr<monitor_t> make_monitor();
  std::shared_ptr<monitor_t> make_monitor(char *name, size_t name_len, int idx, xcb_rectangle_t *rect);

  std::vector<std::shared_ptr<monitor_t>> get_monitors(xcb_connection_t *connection, xcb_window_t root);
}

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

namespace x
{
  typedef xpp::connection<> connection;
  typedef xpp::event::registry<connection &> registry;
  typedef xpp::window<connection &> window;
  typedef xpp::window<xcb_connection_t *> xcb_window;
}

namespace xcb
{
  typedef struct monitor_t monitor_t;
  struct monitor_t
  {
    char name[32] = "NONAME";
    int index = 0;
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
  };

  std::shared_ptr<monitor_t> make_monitor();
  std::shared_ptr<monitor_t> make_monitor(char *name, size_t name_len, int idx, xcb_rectangle_t *rect);

  std::vector<std::shared_ptr<monitor_t>> get_monitors(xcb_connection_t *connection, xcb_window_t root);
}

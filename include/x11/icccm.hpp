#pragma once

#include <xcb/xcb_icccm.h>

#include "common.hpp"

POLYBAR_NS

namespace icccm_util {
  string get_wm_name(xcb_connection_t* conn, xcb_window_t win);
  string get_reply_string(xcb_icccm_get_text_property_reply_t* reply);
}

POLYBAR_NS_END

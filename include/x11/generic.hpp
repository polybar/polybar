#pragma once

#include <xcb/xcb.h>

#include "common.hpp"

POLYBAR_NS

namespace {
  inline bool operator==(const xcb_rectangle_t& a, const xcb_rectangle_t& b) {
    return a.width == b.width && a.height == b.height && a.x == b.x && a.y == b.y;
  }
  inline bool operator!=(const xcb_rectangle_t& a, const xcb_rectangle_t& b) {
    return !(a == b);
  }
  inline bool operator==(const xcb_rectangle_t& a, int b) {
    return a.width == b && a.height == b && a.x == b && a.y == b;
  }
  inline bool operator!=(const xcb_rectangle_t& a, int b) {
    return !(a == b);
  }
}

POLYBAR_NS_END

#pragma once

#include "common.hpp"
#include "components/x11/types.hpp"
#include "components/x11/xlib.hpp"

LEMONBUDDY_NS

namespace xutils {
  static xcb_connection_t* g_connection_ptr = nullptr;
  inline xcb_connection_t* get_connection() {
    if (g_connection_ptr == nullptr) {
      Display* dsp;
      if ((dsp = xlib::get_display()) == nullptr)
        return nullptr;
      XSetEventQueueOwner(dsp, XCBOwnsEventQueue);
      g_connection_ptr = XGetXCBConnection(dsp);
    }
    return g_connection_ptr;
  }

  inline void pack_values(uint32_t mask, const uint32_t* src, uint32_t* dest) {
    for (; mask; mask >>= 1, src++)
      if (mask & 1)
        *dest++ = *src;
  }

  inline void pack_values(uint32_t mask, const xcb_params_cw_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  inline void pack_values(uint32_t mask, const xcb_params_gc_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }
}

LEMONBUDDY_NS_END

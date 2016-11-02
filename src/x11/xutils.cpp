#include "x11/xutils.hpp"
#include "x11/xlib.hpp"

LEMONBUDDY_NS

namespace xutils {
  xcb_connection_t* g_connection_ptr = nullptr;
  xcb_connection_t* get_connection() {
    if (g_connection_ptr == nullptr) {
      Display* dsp;
      if ((dsp = xlib::get_display()) == nullptr)
        return nullptr;
      XSetEventQueueOwner(dsp, XCBOwnsEventQueue);
      g_connection_ptr = XGetXCBConnection(dsp);
    }
    return g_connection_ptr;
  }

  void pack_values(uint32_t mask, const uint32_t* src, uint32_t* dest) {
    for (; mask; mask >>= 1, src++)
      if (mask & 1)
        *dest++ = *src;
  }

  void pack_values(uint32_t mask, const xcb_params_cw_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  void pack_values(uint32_t mask, const xcb_params_gc_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  void pack_values(uint32_t mask, const xcb_params_configure_window_t * src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }
}

LEMONBUDDY_NS_END

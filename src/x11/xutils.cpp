#include "x11/xutils.hpp"
#include "x11/connection.hpp"
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
    for (; mask; mask >>= 1, src++) {
      if (mask & 1) {
        *dest++ = *src;
      }
    }
  }

  void pack_values(uint32_t mask, const xcb_params_cw_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  void pack_values(uint32_t mask, const xcb_params_gc_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  void pack_values(uint32_t mask, const xcb_params_configure_window_t* src, uint32_t* dest) {
    xutils::pack_values(mask, reinterpret_cast<const uint32_t*>(src), dest);
  }

  void visibility_notify(connection& conn, const xcb_window_t& win, xcb_visibility_t state) {
    auto notify = memory_util::make_malloc_ptr<xcb_visibility_notify_event_t>(32);
    notify->response_type = XCB_VISIBILITY_NOTIFY;
    notify->window = win;
    notify->state = state;
    const char* data = reinterpret_cast<const char*>(notify.get());
    conn.send_event(true, win, XCB_EVENT_MASK_NO_EVENT, data);
  }
}

LEMONBUDDY_NS_END

#include <xcb/xcb.h>

#include "components/config.hpp"
#include "utils/memory.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/xlib.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

namespace xutils {
  shared_ptr<int> g_connection_fd;
  shared_ptr<xcb_connection_t> g_connection_ptr;

  shared_ptr<xcb_connection_t> get_connection() {
    if (!g_connection_ptr) {
      shared_ptr<Display> dsp{xlib::get_display()};

      if (dsp) {
        XSetEventQueueOwner(dsp.get(), XCBOwnsEventQueue);
        g_connection_ptr =
            shared_ptr<xcb_connection_t>(XGetXCBConnection(dsp.get()), [=](xcb_connection_t* c) { xcb_disconnect(c); });
      }
    }

    return g_connection_ptr;
  }

  int get_connection_fd() {
    if (!g_connection_fd) {
      auto fd = xcb_get_file_descriptor(get_connection().get());
      g_connection_fd = shared_ptr<int>(new int{fd}, factory_util::fd_deleter{});
    }

    return *g_connection_fd.get();
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

  void compton_shadow_exclude(connection& conn, const xcb_window_t& win) {
    const uint32_t shadow{0};
    conn.change_property(XCB_PROP_MODE_REPLACE, win, _COMPTON_SHADOW, XCB_ATOM_CARDINAL, 32, 1, &shadow);
  }
}

POLYBAR_NS_END

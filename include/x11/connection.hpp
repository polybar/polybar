#pragma once

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <iomanip>
#include <xpp/xpp.hpp>

#include "common.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/types.hpp"
#include "x11/xutils.hpp"

LEMONBUDDY_NS

using xpp_connection = xpp::connection<
#ifdef ENABLE_DAMAGE_EXT
    xpp::damage::extension
#endif
#ifdef ENABLE_RANDR_EXT
#ifdef ENABLE_DAMAGE_EXT
    ,
#endif
    xpp::randr::extension
#endif
#ifdef ENABLE_RENDER_EXT
#ifdef ENABLE_RANDR_EXT
    ,
#endif
    xpp::render::extension
#endif
    >;

class connection : public xpp_connection {
 public:
  explicit connection() {}
  explicit connection(xcb_connection_t* conn) : xpp_connection(conn) {}

  connection& operator=(const connection&) {
    return *this;
  }

  virtual ~connection() {}

  void preload_atoms();

  void query_extensions();

  string id(xcb_window_t w) const;

  xcb_screen_t* screen();

  shared_ptr<xcb_client_message_event_t> make_client_message(
      xcb_atom_t type, xcb_window_t target) const;

  void send_client_message(shared_ptr<xcb_client_message_event_t> message, xcb_window_t target,
      uint32_t event_mask = 0xFFFFFF, bool propagate = false) const;

  void send_dummy_event(xcb_window_t t, uint32_t ev = XCB_EVENT_MASK_STRUCTURE_NOTIFY) const;

  optional<xcb_visualtype_t*> visual_type(xcb_screen_t* screen, int match_depth = 32);

  static string error_str(int error_code);

  void dispatch_event(const shared_ptr<xcb_generic_event_t>& evt) const;

  /**
   * Attach sink to the registry */
  template <typename Sink>
  void attach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.attach(prio, forward<Sink>(sink));
  }

  /**
   * Detach sink from the registry
   */
  template <typename Sink>
  void detach_sink(Sink&& sink, registry::priority prio = 0) {
    m_registry.detach(prio, forward<Sink>(sink));
  }

 protected:
  registry m_registry{*this};
  xcb_screen_t* m_screen = nullptr;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = connection&>
  di::injector<T> configure_connection() {
    return di::make_injector(
        di::bind<>().to(factory::generic_singleton<connection>(xutils::get_connection())));
  }
}

LEMONBUDDY_NS_END

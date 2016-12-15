#pragma once

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <boost/optional.hpp>
#include <iomanip>

#include "common.hpp"
#include "utils/factory.hpp"
#include "x11/extensions.hpp"
#include "x11/registry.hpp"
#include "x11/types.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

using xpp_connection = xpp::connection<XPP_EXTENSION_LIST>;

class connection : public xpp_connection {
 public:
  using make_type = connection&;
  static make_type make();

  explicit connection(xcb_connection_t* conn) : connection(conn, 0) {}
  explicit connection(xcb_connection_t* conn, int connection_fd)
      : xpp_connection(conn), m_connection_fd(connection_fd) {}

  connection& operator=(const connection&) {
    return *this;
  }
  connection(const connection& o) = delete;

  virtual ~connection() {}

  template <typename Event, uint32_t ResponseType>
  void wait_for_response(function<bool(const Event&)> check_event) {
    auto fd = get_file_descriptor();
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    while (true) {
      if (select(fd + 1, &fds, nullptr, nullptr, nullptr) > 0) {
        shared_ptr<xcb_generic_event_t> evt;

        if ((evt = poll_for_event()) && evt->response_type == ResponseType) {
          if (check_event(reinterpret_cast<const xcb_map_notify_event_t&>(*(evt.get())))) {
            break;
          }
        }
      }

      if (connection_has_error()) {
        break;
      }
    }
  }

  void preload_atoms();
  void query_extensions();

  string id(xcb_window_t w) const;

  xcb_screen_t* screen(bool realloc = false);

  void ensure_event_mask(xcb_window_t win, uint32_t event);
  void clear_event_mask(xcb_window_t win);

  shared_ptr<xcb_client_message_event_t> make_client_message(xcb_atom_t type, xcb_window_t target) const;

  void send_client_message(const shared_ptr<xcb_client_message_event_t>& message, xcb_window_t target,
      uint32_t event_mask = 0xFFFFFF, bool propagate = false) const;

  void send_dummy_event(xcb_window_t target, uint32_t event = XCB_EVENT_MASK_STRUCTURE_NOTIFY) const;

  boost::optional<xcb_visualtype_t*> visual_type(xcb_screen_t* screen, int match_depth = 32);

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
  xcb_screen_t* m_screen{nullptr};
  int m_connection_fd{0};
};

POLYBAR_NS_END

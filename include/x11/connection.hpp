#pragma once

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include "common.hpp"
#include "utils/file.hpp"
#include "x11/extensions/all.hpp"
#include "x11/registry.hpp"
#include "x11/types.hpp"

POLYBAR_NS

using xpp_connection = xpp::connection<XPP_EXTENSION_LIST>;

class connection : public xpp_connection {
 public:
  using make_type = connection&;
  static make_type make(xcb_connection_t* conn = nullptr, int conn_fd = 0);

  explicit connection(xcb_connection_t* conn) : connection(conn, 0) {}

  explicit connection(xcb_connection_t* conn, int connection_fd)
      : xpp_connection(conn), m_connection_fd(file_util::make_file_descriptor(connection_fd)) {}

  connection& operator=(const connection&) {
    return *this;
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

  xcb_visualtype_t* visual_type(xcb_screen_t* screen, int match_depth = 32);

  static string error_str(int error_code);

  void dispatch_event(shared_ptr<xcb_generic_event_t>&& evt) const;

  template <typename Event, uint32_t ResponseType>
  void wait_for_response(function<bool(const Event&)> check_event) {
    shared_ptr<xcb_generic_event_t> evt;
    while (!connection_has_error()) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(*m_connection_fd, &fds);

      if (!select(*m_connection_fd + 1, &fds, nullptr, nullptr, nullptr)) {
        continue;
      } else if ((evt = poll_for_event()) == nullptr) {
        continue;
      } else if (evt->response_type != ResponseType) {
        continue;
      } else if (check_event(reinterpret_cast<const Event&>(*(evt.get())))) {
        break;
      }
    }
  }

  /**
   * Attach sink to the registry
   */
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
  shared_ptr<file_descriptor> m_connection_fd;
};

POLYBAR_NS_END

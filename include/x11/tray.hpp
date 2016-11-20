#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "utils/concurrency.hpp"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define TRAY_WM_NAME "Polybar tray window"
#define TRAY_WM_CLASS "tray\0Polybar"

POLYBAR_NS

// fwd declarations
class connection;
struct xembed_data;

namespace graphics_util {
  struct root_pixmap;
}

using root_pixmap = graphics_util::root_pixmap;

// class definition : settings {{{

struct tray_settings {
  tray_settings() = default;
  tray_settings& operator=(const tray_settings& o) = default;

  alignment align{alignment::NONE};
  int16_t orig_x{0};
  int16_t orig_y{0};
  int16_t configured_x{0};
  int16_t configured_y{0};
  uint16_t configured_w{0};
  uint16_t configured_h{0};
  uint16_t configured_slots{0};
  uint16_t width{0};
  uint16_t width_max{0};
  uint16_t height{0};
  uint16_t height_fill{0};
  uint16_t spacing{0};
  uint32_t sibling{0};
  uint32_t background{0};
  bool transparent{false};
};

// }}}
// class definition : tray_client {{{

class tray_client {
 public:
  explicit tray_client(connection& conn, xcb_window_t win, uint16_t w, uint16_t h);

  ~tray_client();

  bool match(const xcb_window_t& win) const;
  bool mapped() const;
  void mapped(bool state);

  xcb_window_t window() const;
  xembed_data* xembed() const;

  void ensure_state() const;
  void reconfigure(int16_t x, int16_t y) const;
  void configure_notify(int16_t x, int16_t y) const;

 protected:
  connection& m_connection;
  xcb_window_t m_window{0};

  shared_ptr<xembed_data> m_xembed;
  stateflag m_mapped{false};

  uint16_t m_width;
  uint16_t m_height;
};

// }}}
// class definition : tray_manager {{{

class tray_manager : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message,
                         evt::configure_request, evt::resize_request, evt::selection_clear, evt::property_notify,
                         evt::reparent_notify, evt::destroy_notify, evt::map_notify, evt::unmap_notify> {
 public:
  explicit tray_manager(connection& conn, const logger& logger);

  ~tray_manager();

  const tray_settings settings() const;

  void bootstrap(tray_settings settings);
  void activate();
  void deactivate();
  void reconfigure();

 protected:
  void reconfigure_window();
  void reconfigure_clients();
  void reconfigure_bg(bool realloc = false);
  void refresh_window();

  void query_atom();
  void create_window();
  void create_bg(bool realloc = false);
  void restack_window();
  void set_wmhints();
  void set_traycolors();

  void acquire_selection();
  void notify_clients();

  void track_selection_owner(xcb_window_t owner);
  void process_docking_request(xcb_window_t win);

  void bar_visibility_change(bool state);

  int16_t calculate_x(uint16_t width) const;
  int16_t calculate_y() const;
  uint16_t calculate_w() const;
  uint16_t calculate_h() const;

  int16_t calculate_client_x(const xcb_window_t& win);
  int16_t calculate_client_y();

  shared_ptr<tray_client> find_client(const xcb_window_t& win) const;
  void remove_client(shared_ptr<tray_client>& client, bool reconfigure = true);
  int mapped_clients() const;

  void handle(const evt::expose& evt);
  void handle(const evt::visibility_notify& evt);
  void handle(const evt::client_message& evt);
  void handle(const evt::configure_request& evt);
  void handle(const evt::resize_request& evt);
  void handle(const evt::selection_clear& evt);
  void handle(const evt::property_notify& evt);
  void handle(const evt::reparent_notify& evt);
  void handle(const evt::destroy_notify& evt);
  void handle(const evt::map_notify& evt);
  void handle(const evt::unmap_notify& evt);

 private:
  connection& m_connection;
  const logger& m_log;
  vector<shared_ptr<tray_client>> m_clients;

  tray_settings m_opts;

  xcb_gcontext_t m_gc{0};
  xcb_pixmap_t m_pixmap{0};
  root_pixmap m_rootpixmap;
  uint16_t m_prevwidth{0};
  uint16_t m_prevheight{0};

  xcb_atom_t m_atom{0};
  xcb_window_t m_tray{0};
  xcb_window_t m_othermanager{0};

  stateflag m_activated{false};
  stateflag m_mapped{false};
  stateflag m_hidden{false};
  stateflag m_sinkattached{false};

  thread m_delayed_activation;

  bool m_restacked{false};

  std::mutex m_mtx;
};

// }}}

namespace {
  /**
   * Configure injection module
   */
  template <class T = unique_ptr<tray_manager>>
  di::injector<T> configure_tray_manager() {
    return di::make_injector(configure_logger(), configure_connection());
  }
}

POLYBAR_NS_END

#pragma once

#include <chrono>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "utils/concurrency.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/tray_client.hpp"

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define TRAY_WM_NAME "Polybar tray window"
#define TRAY_WM_CLASS "tray\0Polybar"

#define TRAY_PLACEHOLDER "<placeholder-systray>"

POLYBAR_NS

namespace chrono = std::chrono;
using namespace std::chrono_literals;

// fwd declarations
class connection;
struct xembed_data;
class background_manager;

struct tray_settings {
  tray_settings() = default;
  tray_settings& operator=(const tray_settings& o) = default;

  alignment align{alignment::NONE};
  bool running{false};
  int orig_x{0};
  int orig_y{0};
  int configured_x{0};
  int configured_y{0};
  unsigned int configured_w{0U};
  unsigned int configured_h{0U};
  unsigned int configured_slots{0U};
  unsigned int width{0U};
  unsigned int width_max{0U};
  unsigned int height{0U};
  unsigned int height_fill{0U};
  unsigned int spacing{0U};
  unsigned int sibling{0U};
  unsigned int background{0U};
  bool transparent{false};
  bool detached{false};
};

class tray_manager
    : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message, evt::configure_request,
          evt::resize_request, evt::selection_clear, evt::property_notify, evt::reparent_notify, evt::destroy_notify,
          evt::map_notify, evt::unmap_notify>,
      public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui::visibility_change, signals::ui::dim_window, signals::ui::update_background> {
 public:
  using make_type = unique_ptr<tray_manager>;
  static make_type make();

  explicit tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, background_manager& back);

  ~tray_manager();

  const tray_settings settings() const;

  void setup(const bar_settings& bar_opts);
  void activate();
  void activate_delayed(chrono::duration<double, std::milli> delay = 1s);
  void deactivate(bool clear_selection = true);
  void reconfigure();

 protected:
  void reconfigure_window();
  void reconfigure_clients();
  void reconfigure_bg(bool realloc = false);
  void refresh_window();
  void redraw_window(bool realloc_bg = false);

  void query_atom();
  void create_window();
  void create_bg(bool realloc = false);
  void restack_window();
  void set_wm_hints();
  void set_tray_colors();

  void acquire_selection();
  void notify_clients();
  void notify_clients_delayed();

  void track_selection_owner(xcb_window_t owner);
  void process_docking_request(xcb_window_t win);

  int calculate_x(unsigned width) const;
  int calculate_y() const;
  unsigned int calculate_w() const;
  unsigned int calculate_h() const;

  int calculate_client_x(const xcb_window_t& win);
  int calculate_client_y();

  bool is_embedded(const xcb_window_t& win) const;
  shared_ptr<tray_client> find_client(const xcb_window_t& win) const;
  void remove_client(shared_ptr<tray_client>& client, bool reconfigure = true);
  void remove_client(xcb_window_t win, bool reconfigure = true);
  unsigned int mapped_clients() const;

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

  bool on(const signals::ui::visibility_change& evt);
  bool on(const signals::ui::dim_window& evt);
  bool on(const signals::ui::update_background& evt);

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  background_manager& m_background;
  vector<shared_ptr<tray_client>> m_clients;

  tray_settings m_opts{};

  xcb_gcontext_t m_gc{0};
  xcb_pixmap_t m_pixmap{0};
  unique_ptr<cairo::surface> m_surface;
  unique_ptr<cairo::context> m_context;

  unsigned int m_prevwidth{0U};
  unsigned int m_prevheight{0U};

  xcb_atom_t m_atom{0};
  xcb_window_t m_tray{0};
  xcb_window_t m_othermanager{0};

  atomic<bool> m_activated{false};
  atomic<bool> m_mapped{false};
  atomic<bool> m_hidden{false};
  atomic<bool> m_acquired_selection{false};

  thread m_delaythread;

  mutex m_mtx{};

  bool m_firstactivation{true};
};

POLYBAR_NS_END

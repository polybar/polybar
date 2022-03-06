#pragma once

#include <atomic>
#include <chrono>
#include <memory>

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
using std::atomic;

// fwd declarations
class connection;
class background_manager;
class bg_slice;

struct tray_settings {
  alignment align{alignment::NONE};
  bool running{false};

  /**
   * Tray window position.
   *
   * Relative to the bar window
   * TODO make relative to inner area
   */
  position pos{0, 0};

  /**
   * Tray offset in pixels.
   */
  position offset{0, 0};

  /**
   * Current dimensions of the tray window.
   */
  size win_size{0, 0};

  /**
   * Dimensions for client windows.
   */
  size client_size{0, 0};

  /**
   * Number of clients currently mapped.
   */
  int num_clients{0};

  // This is the width of the bar window
  // TODO directly read from bar_settings
  unsigned int width_max{0U};

  /**
   * Number of pixels added between tray icons
   */
  unsigned int spacing{0U};
  rgba background{};
  rgba foreground{};
  bool transparent{false};
  bool detached{false};
  bool adaptive{false};

  xcb_window_t bar_window;
};

class tray_manager : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message,
                         evt::configure_request, evt::resize_request, evt::selection_clear, evt::property_notify,
                         evt::reparent_notify, evt::destroy_notify, evt::map_notify, evt::unmap_notify>,
                     public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui::visibility_change, signals::ui::dim_window,
                         signals::ui::update_background, signals::ui_tray::tray_pos_change> {
 public:
  using make_type = unique_ptr<tray_manager>;
  static make_type make(const bar_settings& settings);

  explicit tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, background_manager& back,
      const bar_settings& settings);

  ~tray_manager();

  const tray_settings settings() const;

  void setup();
  void activate();
  void activate_delayed(chrono::duration<double, std::milli> delay = 1s);
  void deactivate(bool clear_selection = true);
  void reconfigure();

 protected:
  void reconfigure_window();
  void reconfigure_clients();
  void reconfigure_bg();
  void refresh_window();
  void redraw_window();

  void query_atom();
  void create_window();
  void create_bg();
  void set_wm_hints();
  void set_tray_colors();

  void acquire_selection();
  void notify_clients();
  void notify_clients_delayed();

  void track_selection_owner(xcb_window_t owner);
  void process_docking_request(xcb_window_t win);

  int calculate_x(unsigned width) const;
  int calculate_y() const;

  unsigned short int calculate_w() const;
  unsigned short int calculate_h() const;

  int calculate_client_x(const xcb_window_t& win);
  int calculate_client_y();

  bool is_embedded(const xcb_window_t& win) const;
  shared_ptr<tray_client> find_client(const xcb_window_t& win) const;
  void remove_client(shared_ptr<tray_client>& client, bool reconfigure = true);
  void remove_client(xcb_window_t win, bool reconfigure = true);
  int mapped_clients() const;
  bool has_mapped_clients() const;

  void handle(const evt::expose& evt) override;
  void handle(const evt::visibility_notify& evt) override;
  void handle(const evt::client_message& evt) override;
  void handle(const evt::configure_request& evt) override;
  void handle(const evt::resize_request& evt) override;
  void handle(const evt::selection_clear& evt) override;
  void handle(const evt::property_notify& evt) override;
  void handle(const evt::reparent_notify& evt) override;
  void handle(const evt::destroy_notify& evt) override;
  void handle(const evt::map_notify& evt) override;
  void handle(const evt::unmap_notify& evt) override;

  bool on(const signals::ui::visibility_change& evt) override;
  bool on(const signals::ui::dim_window& evt) override;
  bool on(const signals::ui::update_background& evt) override;
  bool on(const signals::ui_tray::tray_pos_change& evt) override;

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  background_manager& m_background_manager;
  std::shared_ptr<bg_slice> m_bg_slice;
  vector<shared_ptr<tray_client>> m_clients;

  tray_settings m_opts{};
  const bar_settings& m_bar_opts;

  xcb_gcontext_t m_gc{0};
  xcb_pixmap_t m_pixmap{0};
  unique_ptr<cairo::surface> m_surface;
  unique_ptr<cairo::context> m_context;

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

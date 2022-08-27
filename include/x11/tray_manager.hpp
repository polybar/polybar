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

#define TRAY_PLACEHOLDER "<placeholder-systray>"

POLYBAR_NS

namespace chrono = std::chrono;
using namespace std::chrono_literals;
using std::atomic;

// fwd declarations
class connection;
class bg_slice;

// TODO move mutable options into tray_manager class
struct tray_settings {
  bool running{false};

  /**
   * Specifies the top-left corner of the tray area relative to inner area of the bar.
   */
  position pos{0, 0};

  /**
   * Current dimensions of the tray window.
   *
   * TODO setting almost never used -> refactor
   */
  size win_size{0, 0};

  /**
   * Dimensions for client windows.
   */
  size client_size{0, 0};

  /**
   * Number of clients currently mapped.
   */
  int num_mapped_clients{0};

  /**
   * Number of pixels added between tray icons
   */
  unsigned spacing{0U};
  rgba background{};
  rgba foreground{};

  /**
   * Window ID of tray selection owner.
   *
   * Matches the bar window
   */
  xcb_window_t selection_owner{XCB_NONE};
};

class tray_manager
    : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message, evt::configure_request,
          evt::resize_request, evt::selection_clear, evt::property_notify, evt::reparent_notify, evt::destroy_notify,
          evt::map_notify, evt::unmap_notify>,
      public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui::visibility_change, signals::ui::update_background,
          signals::ui_tray::tray_pos_change, signals::ui_tray::tray_visibility> {
 public:
  using make_type = unique_ptr<tray_manager>;
  static make_type make(const bar_settings& bar_opts);

  explicit tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, const bar_settings& bar_opts);

  ~tray_manager();

  // TODO only expose necessary information (e.g. running)
  const tray_settings settings() const;

  void setup(const string& tray_module_name);
  void activate();
  void activate_delayed(chrono::duration<double, std::milli> delay = 1s);
  void deactivate(bool clear_selection = true);
  void reconfigure();

 protected:
  void reconfigure_window();
  void reconfigure_clients();
  void refresh_window();
  void redraw_window();

  void query_atom();
  void set_tray_colors();

  void acquire_selection();
  void notify_clients();
  void notify_clients_delayed();

  void track_selection_owner(xcb_window_t owner);
  void process_docking_request(xcb_window_t win);

  /**
   * Final x-position of the tray window relative to the very top-left bar window.
   */
  int calculate_x() const;

  /**
   * Final y-position of the tray window relative to the very top-left bar window.
   */
  int calculate_y() const;

  unsigned calculate_w() const;
  unsigned calculate_h() const;

  int calculate_client_y();

  bool is_embedded(const xcb_window_t& win) const;
  tray_client* find_client(const xcb_window_t& win);
  void remove_client(const tray_client& client, bool reconfigure = true);
  void remove_client(xcb_window_t win, bool reconfigure = true);
  int mapped_clients() const;
  bool has_mapped_clients() const;
  bool change_visibility(bool visible);

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
  bool on(const signals::ui::update_background& evt) override;
  bool on(const signals::ui_tray::tray_pos_change& evt) override;
  bool on(const signals::ui_tray::tray_visibility& evt) override;

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  vector<tray_client> m_clients;

  tray_settings m_opts{};
  const bar_settings& m_bar_opts;

  xcb_atom_t m_atom{0};
  xcb_window_t m_othermanager{0};

  atomic<bool> m_activated{false};
  atomic<bool> m_mapped{false};
  atomic<bool> m_hidden{false};
  atomic<bool> m_acquired_selection{false};

  thread m_delaythread;

  // TODO try to remove mutex
  mutable mutex m_mtx{};

  bool m_firstactivation{true};
};

POLYBAR_NS_END

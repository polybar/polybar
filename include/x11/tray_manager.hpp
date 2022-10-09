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

struct tray_settings {
  /**
   * Dimensions for client windows.
   */
  size client_size{0, 0};

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

using on_update = std::function<void(void)>;

class tray_manager : public xpp::event::sink<evt::expose, evt::client_message, evt::configure_request,
                         evt::resize_request, evt::selection_clear, evt::property_notify, evt::reparent_notify,
                         evt::destroy_notify, evt::map_notify, evt::unmap_notify>,
                     public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui::update_background,
                         signals::ui_tray::tray_pos_change, signals::ui_tray::tray_visibility> {
 public:
  explicit tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, const bar_settings& bar_opts,
      on_update on_update);

  ~tray_manager();

  unsigned get_width() const;

  void setup(const config& conf, const string& module_section_name);
  void activate();
  void wait_for_selection(xcb_window_t other);
  void deactivate();
  void reconfigure();

  bool is_active() const;
  bool is_inactive() const;
  bool is_waiting() const;

  bool is_visible() const;

 protected:
  void reconfigure_window();
  void reconfigure_clients();
  void redraw_clients();

  void query_atom();
  void set_tray_colors();
  void set_tray_orientation();
  void set_tray_visual();

  bool acquire_selection(xcb_window_t& other_owner);
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

  int calculate_client_y();

  void update_width();

  bool is_embedded(const xcb_window_t& win);
  tray_client* find_client(const xcb_window_t& win);
  void remove_client(const tray_client& client);
  void remove_client(xcb_window_t win);
  void clean_clients();
  bool change_visibility(bool visible);

  void handle(const evt::expose& evt) override;
  void handle(const evt::client_message& evt) override;
  void handle(const evt::configure_request& evt) override;
  void handle(const evt::resize_request& evt) override;
  void handle(const evt::selection_clear& evt) override;
  void handle(const evt::property_notify& evt) override;
  void handle(const evt::reparent_notify& evt) override;
  void handle(const evt::destroy_notify& evt) override;
  void handle(const evt::map_notify& evt) override;
  void handle(const evt::unmap_notify& evt) override;

  bool on(const signals::ui::update_background& evt) override;
  bool on(const signals::ui_tray::tray_pos_change& evt) override;
  bool on(const signals::ui_tray::tray_visibility& evt) override;

 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  vector<unique_ptr<tray_client>> m_clients;

  tray_settings m_opts{};
  const bar_settings& m_bar_opts;

  const on_update m_on_update;

  enum class state {
    /**
     * The tray manager is completely deactivated and doesn't own any resources.
     */
    INACTIVE = 0,
    /**
     * There is currently another application owning the systray selection and the tray manager waits until the
     * selection becomes available again.
     *
     * The signal receiver is detached in m_othermanager is set
     */
    WAITING,
    /**
     * The tray manager owns the systray selection.
     *
     * The signal receiver is attached
     */
    ACTIVE,
  };
  atomic<state> m_state{state::INACTIVE};

  /**
   * Systray selection atom _NET_SYSTEM_TRAY_Sn
   */
  xcb_atom_t m_atom{XCB_NONE};

  /**
   * Owner of systray selection (if we don't own it)
   */
  xcb_window_t m_othermanager{XCB_NONE};

  /**
   * Specifies the top-left corner of the tray area relative to inner area of the bar.
   */
  position m_pos{0, 0};

  /**
   * Current width of the tray.
   *
   * Caches the value calculated from all mapped tray clients.
   */
  unsigned m_tray_width{0};

  /**
   * Whether the tray is visible
   */
  bool m_hidden{false};

  thread m_delaythread;

  bool m_firstactivation{true};
};

POLYBAR_NS_END

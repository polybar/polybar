#pragma once

#include <xcb/xcb.h>

#include <atomic>
#include <chrono>
#include <memory>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "common.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "utils/concurrency.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/xembed.hpp"

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
class background_manager;
class bg_slice;

namespace legacy_tray {

namespace chrono = std::chrono;
using namespace std::chrono_literals;
using std::atomic;

enum class tray_postition { NONE = 0, LEFT, CENTER, RIGHT, MODULE };

struct tray_settings {
  tray_postition tray_position{tray_postition::NONE};
  bool running{false};
  int rel_x{0};
  int rel_y{0};
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
  rgba background{};
  rgba foreground{};
  bool transparent{false};
  bool detached{false};
};

class tray_client {
 public:
  explicit tray_client(connection& conn, xcb_window_t win, unsigned int w, unsigned int h);
  tray_client(const tray_client& c) = delete;
  tray_client& operator=(tray_client& c) = delete;

  ~tray_client();

  unsigned int width() const;
  unsigned int height() const;
  void clear_window() const;

  bool match(const xcb_window_t& win) const;
  bool mapped() const;
  void mapped(bool state);

  xcb_window_t window() const;

  void query_xembed();
  bool is_xembed_supported() const;
  const xembed::info& get_xembed() const;

  void ensure_state() const;
  void reconfigure(int x, int y) const;
  void configure_notify(int x, int y) const;

 protected:
  connection& m_connection;
  xcb_window_t m_window{0};

  /**
   * Whether the client window supports XEMBED.
   *
   * A tray client can still work when it doesn't support XEMBED.
   */
  bool m_xembed_supported{false};

  /**
   * _XEMBED_INFO of the client window
   */
  xembed::info m_xembed;

  bool m_mapped{false};

  unsigned int m_width;
  unsigned int m_height;
};

class tray_manager : public xpp::event::sink<evt::expose, evt::visibility_notify, evt::client_message,
                         evt::configure_request, evt::resize_request, evt::selection_clear, evt::property_notify,
                         evt::reparent_notify, evt::destroy_notify, evt::map_notify, evt::unmap_notify>,
                     public signal_receiver<SIGN_PRIORITY_TRAY, signals::ui::visibility_change, signals::ui::dim_window,
                         signals::ui::update_background, signals::ui_tray::tray_pos_change>,
                     public non_copyable_mixin,
                     public non_movable_mixin {
 public:
  using make_type = unique_ptr<tray_manager>;
  static make_type make(const bar_settings& settings);

  explicit tray_manager(connection& conn, signal_emitter& emitter, const logger& logger, background_manager& back,
      const bar_settings& settings);

  ~tray_manager();

  const tray_settings settings() const;

  void setup(const config&, const string& tray_module_name);
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
  int calculate_y(bool abspos = true) const;

  unsigned short int calculate_w() const;
  unsigned short int calculate_h() const;

  int calculate_client_x(const xcb_window_t& win);
  int calculate_client_y();

  bool is_embedded(const xcb_window_t& win) const;
  shared_ptr<tray_client> find_client(const xcb_window_t& win) const;
  void remove_client(shared_ptr<tray_client>& client, bool reconfigure = true);
  void remove_client(xcb_window_t win, bool reconfigure = true);
  unsigned int mapped_clients() const;
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

  const bar_settings& m_bar_opts;
};

} // namespace legacy_tray

POLYBAR_NS_END

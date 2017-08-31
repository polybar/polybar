#pragma once

#include "common.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "events/types.hpp"
#include "x11/extensions/fwd.hpp"
#include "x11/types.hpp"

POLYBAR_NS

class logger;

namespace cairo {
  class surface;
  class xcb_surface;
}

class background_manager : public signal_receiver<SIGN_PRIORITY_SCREEN, signals::ui::update_geometry>,
                           public xpp::event::sink<evt::property_notify>
{
 public:
  using make_type = background_manager&;
  static make_type make();

  explicit background_manager(connection& conn, signal_emitter& sig, const logger& log);
  ~background_manager();

  void activate(xcb_window_t window, xcb_rectangle_t rect);
  void deactivate();

  cairo::surface* get_surface() const;

  void handle(const evt::property_notify& evt);
  bool on(const signals::ui::update_geometry&);
 private:
  connection& m_connection;
  signal_emitter& m_sig;
  const logger& m_log;
  xcb_window_t m_window;

  xcb_rectangle_t m_rect{0, 0, 0U, 0U};

  xcb_visualtype_t* m_visual = nullptr;
  xcb_gcontext_t m_gcontext = XCB_NONE;
  xcb_pixmap_t m_pixmap = XCB_NONE;
  unique_ptr<cairo::xcb_surface> m_surface;

  bool m_attached = false;

  void allocate_resources();
  void free_resources();
  void fetch_root_pixmap();

};

POLYBAR_NS_END

#pragma once

#include <xcb/xcb.h>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "common.hpp"
#include "utils/concurrency.hpp"
#include "x11/background_manager.hpp"
#include "x11/xembed.hpp"

/*
 * Manages the lifecycle of a tray client according to the XEMBED protocol
 *
 * Ref: https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html
 */

POLYBAR_NS

// fwd declarations
class connection;

namespace tray {

class client : public non_copyable_mixin, public non_movable_mixin {
 public:
  explicit client(
      const logger& log, connection& conn, xcb_window_t parent, xcb_window_t win, size s, rgba desired_background);
  ~client();

  string name() const;

  unsigned int width() const;
  unsigned int height() const;
  void clear_window() const;

  void update_client_attributes() const;
  void reparent() const;

  bool match(const xcb_window_t& win) const;
  bool mapped() const;
  void mapped(bool state);

  void hidden(bool state);

  bool should_be_mapped() const;

  xcb_window_t embedder() const;
  xcb_window_t client_window() const;

  void query_xembed();
  bool is_xembed_supported() const;
  const xembed::info& get_xembed() const;

  void notify_xembed() const;

  void add_to_save_set() const;

  void ensure_state() const;
  void set_position(int x, int y);
  void configure_notify() const;

  void update_bg() const;

 protected:
  void observe_background();

  const logger& m_log;

  connection& m_connection;

  /**
   * Name of the client window for debugging.
   */
  string m_name{};

  /**
   * Embedder window.
   *
   * The docking client window is reparented to this window.
   * This window is itself a child of the main tray window.
   *
   * This class owns this window and is responsible for creating/destroying it.
   */
  xcb_window_t m_wrapper{XCB_NONE};

  /**
   * Client window.
   *
   * The window itself is owned by the application providing it.
   * This class is responsible for correctly mapping and reparenting it in accordance with the XEMBED protocol.
   */
  xcb_window_t m_client{XCB_NONE};

  /**
   * Whether the client window supports XEMBED.
   *
   * A tray client can still work when it doesn't support XEMBED.
   */
  bool m_xembed_supported{false};

  /**
   * _XEMBED_INFO of the client window
   *
   * Only valid if m_xembed_supported == true
   */
  xembed::info m_xembed{};

  /**
   * Background pixmap of wrapper window
   */
  xcb_pixmap_t m_pixmap{XCB_NONE};

  /**
   * Whether the wrapper window is currently mapped.
   */
  bool m_mapped{false};

  /**
   * Whether the
   */
  bool m_hidden{false};

  size m_size;
  position m_pos{0, 0};

  const rgba m_desired_background;
  const bool m_transparent{m_desired_background.is_transparent()};
  background_manager& m_background_manager;
  shared_ptr<bg_slice> m_bg_slice{nullptr};
  unique_ptr<cairo::context> m_context;
  unique_ptr<cairo::xcb_surface> m_surface;
};

} // namespace tray

POLYBAR_NS_END

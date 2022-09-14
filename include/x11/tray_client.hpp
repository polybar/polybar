#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "utils/concurrency.hpp"
#include "x11/xembed.hpp"

/*
 * Manages the lifecycle of a tray client according to the XEMBED protocol
 *
 * Ref: https://specifications.freedesktop.org/xembed-spec/xembed-spec-latest.html
 */

POLYBAR_NS

// fwd declarations
class connection;

class tray_client : public non_copyable_mixin {
 public:
  explicit tray_client(const logger& log, connection& conn, xcb_window_t tray, xcb_window_t win, size s);
  ~tray_client();

  tray_client(tray_client&&);
  tray_client& operator=(tray_client&&);

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

  xcb_window_t embedder() const;
  xcb_window_t client() const;

  void query_xembed();
  bool is_xembed_supported() const;
  const xembed::info& get_xembed() const;

  void notify_xembed() const;

  void add_to_save_set() const;

  void ensure_state() const;
  void reconfigure(int x, int y) const;
  void configure_notify() const;

 protected:
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
  xembed::info m_xembed;

  // TODO
  bool m_mapped{false};

  bool m_hidden{false};

  size m_size;
};

POLYBAR_NS_END

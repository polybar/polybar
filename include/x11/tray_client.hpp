#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

// fwd declarations
class connection;
struct xembed_data;

class tray_client {
 public:
  explicit tray_client(connection& conn, xcb_window_t win, uint16_t w, uint16_t h);
  tray_client(const tray_client& c) = default;
  tray_client& operator=(tray_client& c) = default;

  ~tray_client();

  uint16_t width() const;
  uint16_t height() const;
  void clear_window() const;

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
  bool m_mapped{false};

  uint16_t m_width;
  uint16_t m_height;
};

POLYBAR_NS_END

#include "x11/winspec.hpp"

#include "x11/connection.hpp"

POLYBAR_NS

winspec::winspec(connection& conn) : m_connection(conn) {}
winspec::winspec(connection& conn, const xcb_window_t& window) : m_connection(conn), m_window(window) {}

winspec::operator xcb_window_t() const {
  return m_window;
}
winspec::operator xcb_rectangle_t() const {
  return {m_x, m_y, m_width, m_height};
}

xcb_window_t winspec::operator<<(const cw_flush& f) {
  std::array<uint32_t, 32> values{};

  if (m_window == XCB_NONE) {
    m_window = m_connection.generate_id();
  }
  if (m_parent == XCB_NONE) {
    m_parent = m_connection.root();
  }

  if (m_width <= 0) {
    m_width = 1;
  }
  if (m_height <= 0) {
    m_height = 1;
  }

  connection::pack_values(m_mask, &m_params, values);

  if (f.checked) {
    m_connection.create_window_checked(
        m_depth, m_window, m_parent, m_x, m_y, m_width, m_height, m_border, m_class, m_visual, m_mask, values.data());
  } else {
    m_connection.create_window(
        m_depth, m_window, m_parent, m_x, m_y, m_width, m_height, m_border, m_class, m_visual, m_mask, values.data());
  }

  return m_window;
}

winspec& winspec::operator<<(const cw_size& size) {
  m_width = size.w;
  m_height = size.h;
  return *this;
}
winspec& winspec::operator<<(const cw_pos& p) {
  m_x = p.x;
  m_y = p.y;
  return *this;
}
winspec& winspec::operator<<(const cw_border& b) {
  m_border = b.border_width;
  return *this;
}
winspec& winspec::operator<<(const cw_class& c) {
  m_class = c.class_;
  return *this;
}
winspec& winspec::operator<<(const cw_parent& p) {
  m_parent = p.parent;
  return *this;
}
winspec& winspec::operator<<(const cw_depth& d) {
  m_depth = d.depth;
  return *this;
}
winspec& winspec::operator<<(const cw_visual& v) {
  m_visual = v.visualid;
  return *this;
}

winspec& winspec::operator<<(const cw_params_back_pixel& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, back_pixel, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_back_pixmap& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, back_pixmap, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_backing_pixel& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, backing_pixel, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_backing_planes& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, backing_planes, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_backing_store& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, backing_store, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_bit_gravity& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, bit_gravity, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_border_pixel& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, border_pixel, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_border_pixmap& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, border_pixmap, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_colormap& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, colormap, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_cursor& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, cursor, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_dont_propagate& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, dont_propagate, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_event_mask& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, event_mask, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_override_redirect& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, override_redirect, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_save_under& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, save_under, p.value);
  return *this;
}
winspec& winspec::operator<<(const cw_params_win_gravity& p) {
  XCB_AUX_ADD_PARAM(&m_mask, &m_params, win_gravity, p.value);
  return *this;
}

POLYBAR_NS_END

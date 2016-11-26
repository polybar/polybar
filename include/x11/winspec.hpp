#pragma once

#include <xcb/xcb_aux.h>

#include "common.hpp"
#include "components/types.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

struct cw_size {
  explicit cw_size(uint16_t w, uint16_t h) : w(w), h(h) {}
  explicit cw_size(struct size size) : w(size.w), h(size.h) {}
  uint16_t w{1};
  uint16_t h{1};
};
struct cw_pos {
  explicit cw_pos(int16_t x, int16_t y) : x(x), y(y) {}
  explicit cw_pos(struct position pos) : x(pos.x), y(pos.y) {}
  int16_t x{0};
  int16_t y{0};
};
struct cw_border {
  explicit cw_border(uint16_t border_width) : border_width(border_width) {}
  uint16_t border_width{0};
};
struct cw_class {
  explicit cw_class(uint16_t class_) : class_(class_) {}
  uint16_t class_{XCB_COPY_FROM_PARENT};
};
struct cw_parent {
  explicit cw_parent(xcb_window_t parent) : parent(parent) {}
  xcb_window_t parent{XCB_NONE};
};
struct cw_depth {
  explicit cw_depth(uint8_t depth) : depth(depth) {}
  uint8_t depth{XCB_COPY_FROM_PARENT};
};
struct cw_visual {
  explicit cw_visual(xcb_visualid_t visualid) : visualid(visualid) {}
  xcb_visualid_t visualid{XCB_COPY_FROM_PARENT};
};
struct cw_mask {
  explicit cw_mask(uint32_t mask) : mask(mask) {}
  const uint32_t mask{0};
};
struct cw_params {
  explicit cw_params(const xcb_params_cw_t* params) : params(params) {}
  const xcb_params_cw_t* params{nullptr};
};
struct cw_params_back_pixel {
  explicit cw_params_back_pixel(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_back_pixmap {
  explicit cw_params_back_pixmap(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_backing_pixel {
  explicit cw_params_backing_pixel(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_backing_planes {
  explicit cw_params_backing_planes(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_backing_store {
  explicit cw_params_backing_store(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_bit_gravity {
  explicit cw_params_bit_gravity(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_border_pixel {
  explicit cw_params_border_pixel(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_border_pixmap {
  explicit cw_params_border_pixmap(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_colormap {
  explicit cw_params_colormap(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_cursor {
  explicit cw_params_cursor(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_dont_propagate {
  explicit cw_params_dont_propagate(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_event_mask {
  explicit cw_params_event_mask(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_override_redirect {
  explicit cw_params_override_redirect(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_save_under {
  explicit cw_params_save_under(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_params_win_gravity {
  explicit cw_params_win_gravity(uint32_t value) : value(value) {}
  uint32_t value{0};
};
struct cw_flush {
  explicit cw_flush(bool checked = false) : checked(checked) {}
  bool checked{false};
};

/**
 * Create X window
 *
 * Example usage:
 * @code cpp
 *   auto win = winspec(m_connection)
 *     << cw_size(100, 200)
 *     << cw_pos(10, -20)
 *     << cw_border(9)
 *     << cw_class(XCB_WINDOW_CLASS_INPUT_ONLY)
 *     << cw_parent(0x000110a);
 *     << cw_flush(false);
 * @endcode
 */
class winspec {
 public:
  explicit winspec(connection& conn);
  explicit winspec(connection& conn, const xcb_window_t& window);

  explicit operator xcb_window_t() const;
  explicit operator xcb_rectangle_t() const;

  xcb_window_t operator<<(const cw_flush& f);

  winspec& operator<<(const cw_size& size);
  winspec& operator<<(const cw_pos& p);
  winspec& operator<<(const cw_border& b);
  winspec& operator<<(const cw_class& c);
  winspec& operator<<(const cw_parent& p);
  winspec& operator<<(const cw_depth& d);
  winspec& operator<<(const cw_visual& v);
  winspec& operator<<(const cw_params_back_pixel& p);
  winspec& operator<<(const cw_params_back_pixmap& p);
  winspec& operator<<(const cw_params_backing_pixel& p);
  winspec& operator<<(const cw_params_backing_planes& p);
  winspec& operator<<(const cw_params_backing_store& p);
  winspec& operator<<(const cw_params_bit_gravity& p);
  winspec& operator<<(const cw_params_border_pixel& p);
  winspec& operator<<(const cw_params_border_pixmap& p);
  winspec& operator<<(const cw_params_colormap& p);
  winspec& operator<<(const cw_params_cursor& p);
  winspec& operator<<(const cw_params_dont_propagate& p);
  winspec& operator<<(const cw_params_event_mask& p);
  winspec& operator<<(const cw_params_override_redirect& p);
  winspec& operator<<(const cw_params_save_under& p);
  winspec& operator<<(const cw_params_win_gravity& p);

 protected:
  connection& m_connection;

  xcb_window_t m_window{XCB_NONE};
  uint32_t m_parent{XCB_NONE};
  uint8_t m_depth{XCB_COPY_FROM_PARENT};
  uint16_t m_class{XCB_COPY_FROM_PARENT};
  xcb_visualid_t m_visual{XCB_COPY_FROM_PARENT};
  int16_t m_x{0};
  int16_t m_y{0};
  uint16_t m_width{1U};
  uint16_t m_height{1U};
  uint16_t m_border{0};
  uint32_t m_mask{0};
  xcb_params_cw_t m_params{};
};

POLYBAR_NS_END

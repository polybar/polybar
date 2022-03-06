#pragma once

#include <xcb/xcb.h>

#include "common.hpp"
#include "components/types.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

struct cw_size {
  explicit cw_size(unsigned short int w, unsigned short int h) : w(w), h(h) {}
  explicit cw_size(struct size size) : w(size.w), h(size.h) {}
  unsigned short int w{1};
  unsigned short int h{1};
};
struct cw_pos {
  explicit cw_pos(short int x, short int y) : x(x), y(y) {}
  explicit cw_pos(struct position pos) : x(pos.x), y(pos.y) {}
  short int x{0};
  short int y{0};
};
struct cw_border {
  explicit cw_border(unsigned short int border_width) : border_width(border_width) {}
  unsigned short int border_width{0};
};
struct cw_class {
  explicit cw_class(unsigned short int class_) : class_(class_) {}
  unsigned short int class_{XCB_COPY_FROM_PARENT};
};
struct cw_parent {
  explicit cw_parent(xcb_window_t parent) : parent(parent) {}
  xcb_window_t parent{XCB_NONE};
};
struct cw_depth {
  explicit cw_depth(unsigned char depth) : depth(depth) {}
  unsigned char depth{XCB_COPY_FROM_PARENT};
};
struct cw_visual {
  explicit cw_visual(xcb_visualid_t visualid) : visualid(visualid) {}
  xcb_visualid_t visualid{XCB_COPY_FROM_PARENT};
};
struct cw_mask {
  explicit cw_mask(unsigned int mask) : mask(mask) {}
  const unsigned int mask{0};
};
struct cw_params {
  explicit cw_params(const xcb_params_cw_t* params) : params(params) {}
  const xcb_params_cw_t* params{nullptr};
};
struct cw_params_back_pixel {
  explicit cw_params_back_pixel(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_back_pixmap {
  explicit cw_params_back_pixmap(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_backing_pixel {
  explicit cw_params_backing_pixel(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_backing_planes {
  explicit cw_params_backing_planes(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_backing_store {
  explicit cw_params_backing_store(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_bit_gravity {
  explicit cw_params_bit_gravity(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_border_pixel {
  explicit cw_params_border_pixel(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_border_pixmap {
  explicit cw_params_border_pixmap(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_colormap {
  explicit cw_params_colormap(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_cursor {
  explicit cw_params_cursor(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_dont_propagate {
  explicit cw_params_dont_propagate(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_event_mask {
  explicit cw_params_event_mask(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_override_redirect {
  explicit cw_params_override_redirect(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_save_under {
  explicit cw_params_save_under(unsigned int value) : value(value) {}
  unsigned int value{0};
};
struct cw_params_win_gravity {
  explicit cw_params_win_gravity(unsigned int value) : value(value) {}
  unsigned int value{0};
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
  unsigned int m_parent{XCB_NONE};
  unsigned char m_depth{XCB_COPY_FROM_PARENT};
  unsigned short int m_class{XCB_COPY_FROM_PARENT};
  xcb_visualid_t m_visual{XCB_COPY_FROM_PARENT};
  short int m_x{0};
  short int m_y{0};
  unsigned short int m_width{1U};
  unsigned short int m_height{1U};
  unsigned short int m_border{0};
  uint32_t m_mask{0};
  xcb_params_cw_t m_params{};
};

POLYBAR_NS_END

#include <xcb/xcb_icccm.h>

#include "x11/window.hpp"
#include "x11/xutils.hpp"

LEMONBUDDY_NS

window window::create_checked(int16_t x, int16_t y, uint16_t w, uint16_t h) {
  if (*this == XCB_NONE) {
    resource(connection(), connection().generate_id());
  }

  auto root = connection().screen()->root;
  auto copy = XCB_COPY_FROM_PARENT;
  connection().create_window_checked(copy, *this, root, x, y, w, h, 0, copy, copy, 0, nullptr);

  return *this;
}

window window::create_checked(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t mask, const xcb_params_cw_t* p) {
  if (*this == XCB_NONE) {
    resource(connection(), connection().generate_id());
  }

  auto root = connection().screen()->root;
  auto copy = XCB_COPY_FROM_PARENT;
  uint32_t values[16];
  xutils::pack_values(mask, p, values);
  connection().create_window_checked(copy, *this, root, x, y, w, h, 0, copy, copy, mask, values);

  return *this;
}

window window::create_checked(uint16_t w, uint16_t h, uint32_t mask, const xcb_params_cw_t* p) {
  return create_checked(0, 0, w, h, mask, p);
}

window window::reconfigure_geom(uint16_t w, uint16_t h, int16_t x, int16_t y) {
  uint32_t mask = 0;
  uint32_t values[7];

  xcb_params_configure_window_t params;
  XCB_AUX_ADD_PARAM(&mask, &params, width, w);
  XCB_AUX_ADD_PARAM(&mask, &params, height, h);
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  xutils::pack_values(mask, &params, values);
  connection().configure_window_checked(*this, mask, values);

  return *this;
}

window window::reconfigure_pos(int16_t x, int16_t y) {
  uint32_t mask = 0;
  uint32_t values[7];

  xcb_params_configure_window_t params;
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  xutils::pack_values(mask, &params, values);
  connection().configure_window_checked(*this, mask, values);

  return *this;
}

void window::redraw() {
  xutils::visibility_notify(connection(), *this, XCB_VISIBILITY_FULLY_OBSCURED);
  xutils::visibility_notify(connection(), *this, XCB_VISIBILITY_UNOBSCURED);
  connection().flush();
}

LEMONBUDDY_NS_END

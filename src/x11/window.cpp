#include <xcb/xcb_icccm.h>

#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/window.hpp"
#include "x11/xutils.hpp"

#include "components/types.hpp"

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
  uint32_t values[2];

  xcb_params_configure_window_t params;
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  xutils::pack_values(mask, &params, values);
  connection().configure_window_checked(*this, mask, values);

  return *this;
}

window window::reconfigure_struts(const monitor_t& mon, uint16_t w, uint16_t h, int16_t x, int16_t y, bool bottom) {
  auto& conn = connection();
  auto root = conn.screen()->root;
  auto geom = conn.get_geometry(root);

  uint32_t none{0};
  uint32_t strut[12]{none};

  auto bx = geom->x + mon->x;
  auto x1 = bx + x;
  auto x2 = bx + x + w;

  if (bottom) {
    auto by = geom->y + geom->height - mon->h - mon->y;
    strut[static_cast<int>(strut::BOTTOM)] = math_util::cap<int>(by + h + y, 0, by + mon->h);
    strut[static_cast<int>(strut::BOTTOM_START_X)] = math_util::cap<int>(x1, bx, bx + mon->w);
    strut[static_cast<int>(strut::BOTTOM_END_X)] = math_util::cap<int>(x2, bx, bx + mon->w);
  } else {
    auto by = geom->y + mon->y;
    strut[static_cast<int>(strut::TOP)] = math_util::cap<int>(by + h + y, 0, by + mon->h);
    strut[static_cast<int>(strut::TOP_START_X)] = math_util::cap<int>(x1, bx, bx + mon->w);
    strut[static_cast<int>(strut::TOP_END_X)] = math_util::cap<int>(x2, bx, bx + mon->w);
  }

  conn.change_property(XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT, XCB_ATOM_CARDINAL, 32, 4, strut);
  conn.change_property(XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, 12, strut);

  return *this;
}

void window::redraw() {
  xutils::visibility_notify(connection(), *this, XCB_VISIBILITY_FULLY_OBSCURED);
  xutils::visibility_notify(connection(), *this, XCB_VISIBILITY_UNOBSCURED);
  connection().flush();
}

LEMONBUDDY_NS_END

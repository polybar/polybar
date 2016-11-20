#include <xcb/xcb_icccm.h>

#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/window.hpp"
#include "x11/xutils.hpp"

#include "x11/color.hpp"
#include "components/types.hpp"

POLYBAR_NS

/**
 * Create window and check for errors
 */
window window::create_checked(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t mask, const xcb_params_cw_t* p) {
  auto conn = connection();

  if (*this == XCB_NONE) {
    *this = conn.generate_id();
  }

  auto root = conn.screen()->root;
  auto copy = XCB_COPY_FROM_PARENT;
  uint32_t values[16]{0};
  xutils::pack_values(mask, p, values);
  conn.create_window_checked(copy, *this, root, x, y, w, h, 0, copy, copy, mask, values);

  return *this;
}

/**
 * Change the window event mask
 */
window window::change_event_mask(uint32_t mask) {
  change_attributes_checked(XCB_CW_EVENT_MASK, &mask);
  return *this;
}

/**
 * Add given event to the event mask unless already added
 */
window window::ensure_event_mask(uint32_t event) {
  auto attributes = get_attributes();

  if ((attributes->your_event_mask & event) != event) {
    change_event_mask(attributes->your_event_mask | event);
  }

  return *this;
}

/**
 * Reconfigure the window geometry
 */
window window::reconfigure_geom(uint16_t w, uint16_t h, int16_t x, int16_t y) {
  uint32_t mask{0};
  uint32_t values[7]{0};

  xcb_params_configure_window_t params;
  XCB_AUX_ADD_PARAM(&mask, &params, width, w);
  XCB_AUX_ADD_PARAM(&mask, &params, height, h);
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  xutils::pack_values(mask, &params, values);
  configure_checked(mask, values);

  return *this;
}

/**
 * Reconfigure the window position
 */
window window::reconfigure_pos(int16_t x, int16_t y) {
  uint32_t mask{0};
  uint32_t values[2]{0};

  xcb_params_configure_window_t params;
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  xutils::pack_values(mask, &params, values);
  configure_checked(mask, values);

  return *this;
}

/**
 * Reconfigure the windows ewmh strut
 */
window window::reconfigure_struts(uint16_t w, uint16_t h, int16_t x, bool bottom) {
  auto& conn = connection();

  uint32_t none{0};
  uint32_t values[12]{none};

  if (bottom) {
    values[static_cast<int>(strut::BOTTOM)] = h;
    values[static_cast<int>(strut::BOTTOM_START_X)] = x;
    values[static_cast<int>(strut::BOTTOM_END_X)] = x + w - 1;
  } else {
    values[static_cast<int>(strut::TOP)] = h;
    values[static_cast<int>(strut::TOP_START_X)] = x;
    values[static_cast<int>(strut::TOP_END_X)] = x + w - 1;
  }

  conn.change_property_checked(XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT, XCB_ATOM_CARDINAL, 32, 4, values);
  conn.change_property_checked(XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, 12, values);

  return *this;
}

/**
 * Trigger redraw by toggling visibility state
 */
void window::redraw() {
  auto conn = connection();
  xutils::visibility_notify(conn, *this, XCB_VISIBILITY_FULLY_OBSCURED);
  xutils::visibility_notify(conn, *this, XCB_VISIBILITY_UNOBSCURED);
  conn.flush();
}

POLYBAR_NS_END

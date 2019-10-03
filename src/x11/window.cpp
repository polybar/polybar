#include "components/types.hpp"
#include "utils/memory.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/randr.hpp"
#include "x11/window.hpp"

POLYBAR_NS

/**
 * Reconfigure the window geometry
 */
window window::reconfigure_geom(unsigned short int w, unsigned short int h, short int x, short int y) {
  unsigned int mask{0};
  unsigned int values[7]{0};

  xcb_params_configure_window_t params{};
  XCB_AUX_ADD_PARAM(&mask, &params, width, w);
  XCB_AUX_ADD_PARAM(&mask, &params, height, h);
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  connection::pack_values(mask, &params, values);
  configure_checked(mask, values);

  return *this;
}

/**
 * Reconfigure the window position
 */
window window::reconfigure_pos(short int x, short int y) {
  unsigned int mask{0};
  unsigned int values[2]{0};

  xcb_params_configure_window_t params{};
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  connection::pack_values(mask, &params, values);
  configure_checked(mask, values);

  return *this;
}

/**
 * Reconfigure the windows ewmh strut
 */
window window::reconfigure_struts(unsigned short int w, unsigned short int h, short int x, bool bottom) {
  unsigned int none{0};
  unsigned int values[12]{none};

  if (bottom) {
    values[static_cast<int>(strut::BOTTOM)] = h;
    values[static_cast<int>(strut::BOTTOM_START_X)] = x;
    values[static_cast<int>(strut::BOTTOM_END_X)] = x + w - 1;
  } else {
    values[static_cast<int>(strut::TOP)] = h;
    values[static_cast<int>(strut::TOP_START_X)] = x;
    values[static_cast<int>(strut::TOP_END_X)] = x + w - 1;
  }

  connection().change_property_checked(XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT, XCB_ATOM_CARDINAL, 32, 4, values);
  connection().change_property_checked(
      XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, 12, values);

  return *this;
}

POLYBAR_NS_END

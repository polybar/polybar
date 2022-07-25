#include "x11/window.hpp"

#include "components/types.hpp"
#include "utils/memory.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

/**
 * Reconfigure the window geometry
 */
window window::reconfigure_geom(unsigned short int w, unsigned short int h, short int x, short int y) {
  uint32_t mask{0};
  std::array<uint32_t, 32> values{};

  xcb_params_configure_window_t params{};
  XCB_AUX_ADD_PARAM(&mask, &params, width, w);
  XCB_AUX_ADD_PARAM(&mask, &params, height, h);
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  connection::pack_values(mask, &params, values);
  configure_checked(mask, values.data());

  return *this;
}

/**
 * Reconfigure the window position
 */
window window::reconfigure_pos(short int x, short int y) {
  uint32_t mask{0};
  std::array<uint32_t, 32> values{};

  xcb_params_configure_window_t params{};
  XCB_AUX_ADD_PARAM(&mask, &params, x, x);
  XCB_AUX_ADD_PARAM(&mask, &params, y, y);

  connection::pack_values(mask, &params, values);
  configure_checked(mask, values.data());

  return *this;
}

/**
 * Reconfigure the windows ewmh strut
 *
 * Ref: https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm45381391268672
 *
 * @param w Width of the bar window
 * @param strut Size of the reserved space. Height of the window, corrected for unaligned monitors
 * @param x The absolute x-position of the bar window (top-left corner)
 * @param bottom Whether the bar is at the bottom of the screen
 */
window window::reconfigure_struts(uint32_t w, uint32_t strut, uint32_t x, bool bottom) {
  std::array<uint32_t, 12> values{};

  uint32_t end_x = std::max<int>(0, x + w - 1);

  if (bottom) {
    values[to_integral(strut::BOTTOM)] = strut;
    values[to_integral(strut::BOTTOM_START_X)] = x;
    values[to_integral(strut::BOTTOM_END_X)] = end_x;
  } else {
    values[to_integral(strut::TOP)] = strut;
    values[to_integral(strut::TOP_START_X)] = x;
    values[to_integral(strut::TOP_END_X)] = end_x;
  }

  connection().change_property_checked(
      XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT, XCB_ATOM_CARDINAL, 32, 4, values.data());
  connection().change_property_checked(
      XCB_PROP_MODE_REPLACE, *this, _NET_WM_STRUT_PARTIAL, XCB_ATOM_CARDINAL, 32, 12, values.data());

  return *this;
}

POLYBAR_NS_END

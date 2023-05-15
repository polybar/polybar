#include "utils/restack.hpp"

POLYBAR_NS

namespace restack_util {

/**
 * Restacks the given window relative to a given sibling with some stack order (above, below)
 *
 * Both windows need to be siblings (have the same parent window).
 *
 * @param win The window to restack
 * @param sibling The window relative to which restacking happens
 * @param stack_mode The restacking mode (above, below)
 * @throw Some xpp error if restacking fails
 */
void restack_relative(connection& conn, xcb_window_t win, xcb_window_t sibling, xcb_stack_mode_t stack_mode) {
  const unsigned int value_mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;
  const std::array<uint32_t, 2> value_list = {sibling, stack_mode};
  conn.configure_window_checked(win, value_mask, value_list.data());
}

string stack_mode_to_string(xcb_stack_mode_t mode) {
  switch (mode) {
    case XCB_STACK_MODE_ABOVE:
      return "ABOVE";
    case XCB_STACK_MODE_BELOW:
      return "BELOW";
    case XCB_STACK_MODE_TOP_IF:
      return "TOP_IF";
    case XCB_STACK_MODE_BOTTOM_IF:
      return "BOTTOM_IF";
    case XCB_STACK_MODE_OPPOSITE:
      return "OPPOSITE";
  }

  return "UNKNOWN";
}

/**
 * @return true iff the two given windows are sibings (are different windows and have same parent).
 */
bool are_siblings(connection& conn, xcb_window_t win, xcb_window_t sibling) {
  if (win == XCB_NONE || sibling == XCB_NONE || win == sibling) {
    return false;
  }

  auto win_tree = conn.query_tree(win);
  auto sibling_tree = conn.query_tree(sibling);
  return win_tree->parent == sibling_tree->parent;
}

/**
 * "bottom" restack strategy.
 *
 * Moves the bar window to the bottom of the window stack
 */
params get_bottom_params(connection& conn, xcb_window_t bar_window) {
  auto children = conn.query_tree(conn.root()).children();
  if (children.begin() != children.end() && *children.begin() != bar_window) {
    return {*children.begin(), XCB_STACK_MODE_BELOW};
  }

  return NONE_PARAMS;
}

/**
 * EWMH restack strategy.
 *
 * Moves the bar window above the WM meta window (_NET_SUPPORTING_WM_CHECK).
 * This window is generally towards the bottom of the window stack, but still above other windows that could interfere.
 *
 * @see ewmh_util::get_ewmh_meta_window
 */
params get_ewmh_params(connection& conn) {
  if (auto meta_window = ewmh_util::get_ewmh_meta_window(conn.root())) {
    return {meta_window, XCB_STACK_MODE_ABOVE};
  }

  return NONE_PARAMS;
}

/**
 * Generic restack stratgey.
 *
 * Tries to provide the best WM-agnostic restacking.
 *
 * Currently tries to the following stratgies in order:
 * * ewmh
 * * bottom
 */
params get_generic_params(connection& conn, xcb_window_t bar_window) {
  auto ewmh_params = get_ewmh_params(conn);

  if (are_siblings(conn, bar_window, ewmh_params.first)) {
    return ewmh_params;
  }

  auto bottom_params = get_bottom_params(conn, bar_window);

  if (are_siblings(conn, bar_window, bottom_params.first)) {
    return bottom_params;
  }

  return NONE_PARAMS;
}
} // namespace restack_util

POLYBAR_NS_END

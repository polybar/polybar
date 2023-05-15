#include "utils/restack.hpp"

POLYBAR_NS

namespace restack_util {

static constexpr params NONE_PARAMS = {XCB_NONE, XCB_STACK_MODE_ABOVE};

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
std::pair<xcb_window_t, xcb_stack_mode_t> get_bottom_params(connection& conn, xcb_window_t bar_window) {
  auto children = conn.query_tree(conn.root()).children();
  if (children.begin() != children.end() && *children.begin() != bar_window) {
    return {*children.begin(), XCB_STACK_MODE_BELOW};
  }

  return NONE_PARAMS;
}

/**
 * Generic restack stratgey.
 *
 * Tries to provide the best WM-agnostic restacking.
 *
 * Currently tries to the following stratgies in order:
 * * bottom
 */
std::pair<xcb_window_t, xcb_stack_mode_t> get_generic_params(connection& conn, xcb_window_t bar_window) {
  auto [sibling, mode] = get_bottom_params(conn, bar_window);

  if (are_siblings(conn, bar_window, sibling)) {
    return {sibling, mode};
  }

  return NONE_PARAMS;
}
} // namespace restack_util

POLYBAR_NS_END

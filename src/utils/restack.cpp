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
}

POLYBAR_NS_END

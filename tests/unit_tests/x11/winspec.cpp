#include "utils/string.cpp"
#include "x11/atoms.cpp"
#include "x11/connection.cpp"
#include "x11/winspec.hpp"
#include "x11/xutils.cpp"
#include "x11/xlib.cpp"

int main() {
  using namespace polybar;

  "cw_create"_test = [] {
    connection& conn{configure_connection().create<connection&>()};
    auto id = conn.generate_id();

    // clang-format off
    auto win = winspec(conn, id)
      << cw_size(100, 200)
      << cw_pos(10, -20)
      << cw_border(9)
      << cw_class(XCB_WINDOW_CLASS_INPUT_ONLY)
      << cw_parent(0x000110a)
      ;
    // clang-format on

    expect(static_cast<xcb_window_t>(win) == id);

    xcb_rectangle_t rect{static_cast<xcb_rectangle_t>(win)};
    expect(rect.width == 100);
    expect(rect.height == 200);
    expect(rect.x == 10);
    expect(rect.y == -20);
  };
}

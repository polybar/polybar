#include "x11/window.hpp"

int main() {
  using namespace polybar;

  "cw_create"_test = [] {
    // clang-format off
    auto win = winspec()
      << cw_size(100, 200)
      << cw_pos(10, -20)
      << cw_border(9)
      << cw_class(XCB_WINDOW_CLASS_INPUT_ONLY)
      << cw_parent(0x000110a)
      ;
    // clang-format on

    expect(win.width == 100);
    expect(win.height == 200);
    expect(win.x == 10);
    expect(win.y == -20);
    expect(win.border_width == 9);
    expect(win.class_ == XCB_WINDOW_CLASS_INPUT_ONLY);
    expect(win.parent == 0x000110a);
  };
}

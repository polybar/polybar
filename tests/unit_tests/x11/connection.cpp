#include "utils/string.cpp"
#include "x11/atoms.cpp"
#include "x11/connection.cpp"
#include "x11/xutils.cpp"
#include "x11/xlib.cpp"

int main() {
  using namespace polybar;

  "id"_test = [] {
    connection& conn{configure_connection().create<connection&>()};
    expect(conn.id(static_cast<xcb_window_t>(0x12345678)) == "0x12345678");
  };
}

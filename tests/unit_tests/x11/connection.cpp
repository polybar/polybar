#include "x11/connection.hpp"

int main() {
  using namespace lemonbuddy;

  "id"_test = [] {
    connection& conn{connection::configure().create<connection&>()};
    expect(conn.id(static_cast<xcb_window_t>(0x12345678)) == "0x12345678");
  };
}

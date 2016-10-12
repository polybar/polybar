#include <lemonbuddy/components/x11/window.hpp>
#include "../../unit_test.hpp"

using namespace lemonbuddy;

class test_window : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_window);
  CPPUNIT_TEST(test_cw_create);
  CPPUNIT_TEST_SUITE_END();

  void test_cw_create() {
    // clang-format off
    // auto win = winspec()
    //   << cw_size(100, 200)
    //   << cw_pos(10, -20)
    //   << cw_border(9)
    //   << cw_class(XCB_WINDOW_CLASS_INPUT_ONLY)
    //   << cw_parent(0x000110a)
    //   ;
    // clang-format on

    // CPPUNIT_ASSERT_EQUAL(win.width, uint16_t{100});
    // CPPUNIT_ASSERT_EQUAL(win.height, uint16_t{200});
    // CPPUNIT_ASSERT_EQUAL(win.x, int16_t{10});
    // CPPUNIT_ASSERT_EQUAL(win.y, int16_t{-20});
    // CPPUNIT_ASSERT_EQUAL(win.border_width, uint16_t{9});
    // CPPUNIT_ASSERT_EQUAL(win.class_, uint16_t{XCB_WINDOW_CLASS_INPUT_ONLY});
    // CPPUNIT_ASSERT_EQUAL(win.parent, xcb_window_t{0x000110a});
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_window);

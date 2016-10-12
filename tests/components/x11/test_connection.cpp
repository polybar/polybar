#include <lemonbuddy/components/x11/connection.hpp>

#include "../../unit_test.hpp"

using namespace lemonbuddy;

class test_connection : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_connection);
  CPPUNIT_TEST(test_id);
  CPPUNIT_TEST_SUITE_END();

  void test_id() {
    CPPUNIT_ASSERT_EQUAL(string{"0x12345678"}, m_connection.id(static_cast<xcb_window_t>(0x12345678)));
  }

  connection& m_connection = connection::configure().create<connection&>();
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_connection);

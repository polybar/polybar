#include <iomanip>
#include <lemonbuddy/utils/scope.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;

class test_scope : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_scope);
  CPPUNIT_TEST(test_on_exit);
  CPPUNIT_TEST_SUITE_END();

  void test_on_exit() {
    auto flag = false;
    {
      CPPUNIT_ASSERT_EQUAL(false, flag);
      auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
      CPPUNIT_ASSERT_EQUAL(false, flag);
      {
        auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
      }
      CPPUNIT_ASSERT_EQUAL(true, flag);
      flag = false;
    }
    CPPUNIT_ASSERT_EQUAL(true, flag);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_scope);

#include <lemonbuddy/components/logger.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;

class test_logger : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_logger);
  CPPUNIT_TEST(test_output);
  CPPUNIT_TEST_SUITE_END();

  void test_output() {
    auto l = logger::configure<logger>(loglevel::TRACE).create<logger>();
    l.err("error");
    l.warn("warning");
    l.info("info");
    l.trace("trace");
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_logger);

#include <csignal>
#include <lemonbuddy/components/x11/color.hpp>

#include "../../unit_test.hpp"

using namespace lemonbuddy;

class test_draw : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_draw);
  CPPUNIT_TEST(test_color);
  CPPUNIT_TEST(test_cache);
  CPPUNIT_TEST(test_predefined);
  CPPUNIT_TEST(test_parse);
  CPPUNIT_TEST_SUITE_END();

  void test_color() {
    color test{"#33990022"};
    CPPUNIT_ASSERT_EQUAL(string{"#33990022"}, test.hex());
    CPPUNIT_ASSERT_EQUAL(string{"#1E0006"}, test.rgb());
  }

  void test_cache() {
    CPPUNIT_ASSERT_EQUAL(size_t{0}, g_colorstore.size());
    auto c1 = color::parse("#100");
    CPPUNIT_ASSERT_EQUAL(size_t{1}, g_colorstore.size());
    auto c2 = color::parse("#200");
    CPPUNIT_ASSERT_EQUAL(size_t{2}, g_colorstore.size());
    auto c3 = color::parse("#200");
    CPPUNIT_ASSERT_EQUAL(size_t{2}, g_colorstore.size());
    CPPUNIT_ASSERT_EQUAL(c1.value(), g_colorstore.find("#100")->second.value());
  }

  void test_predefined() {
    CPPUNIT_ASSERT_EQUAL(string{"#FF000000"}, g_colorblack.hex());
    CPPUNIT_ASSERT_EQUAL(string{"#FFFFFFFF"}, g_colorwhite.hex());
  }

  void test_parse() {
    CPPUNIT_ASSERT_EQUAL(string{"#FFFF9900"}, color::parse("#ff9900", g_colorblack).hex());
    CPPUNIT_ASSERT_EQUAL(string{"#FFFFFFFF"}, color::parse("invalid", g_colorwhite).hex());
    CPPUNIT_ASSERT_EQUAL(string{"#1E0006"}, color::parse("33990022", g_colorwhite).rgb());
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_draw);

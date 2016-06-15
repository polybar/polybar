#include <iomanip>
#include <lemonbuddy/utils/string.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;

class test_string : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_string);
  CPPUNIT_TEST(test_upper);
  CPPUNIT_TEST(test_lower);
  CPPUNIT_TEST(test_compare);
  CPPUNIT_TEST(test_replace);
  CPPUNIT_TEST(test_replace_all);
  CPPUNIT_TEST(test_squeeze);
  CPPUNIT_TEST(test_strip);
  CPPUNIT_TEST(test_trim);
  CPPUNIT_TEST(test_join);
  CPPUNIT_TEST(test_split_into);
  CPPUNIT_TEST(test_split);
  CPPUNIT_TEST(test_find_nth);
  CPPUNIT_TEST(test_from_stream);
  CPPUNIT_TEST(test_hash);
  CPPUNIT_TEST_SUITE_END();

  void test_upper() {
    CPPUNIT_ASSERT_EQUAL(string{"FOO"}, string_util::upper("FOO"));
  }

  void test_lower() {
    CPPUNIT_ASSERT_EQUAL(string{"bar"}, string_util::lower("BAR"));
  }

  void test_compare() {
    CPPUNIT_ASSERT_EQUAL(true, string_util::compare("foo", "foo"));
    CPPUNIT_ASSERT_EQUAL(false, string_util::compare("foo", "bar"));
  }

  void test_replace() {
    CPPUNIT_ASSERT_EQUAL(string{"Foo bxr baz"}, string_util::replace("Foo bar baz", "a", "x"));
  }

  void test_replace_all() {
    CPPUNIT_ASSERT_EQUAL(string{"Foo bxr bxz"}, string_util::replace_all("Foo bar baz", "a", "x"));
  }

  void test_squeeze() {
    CPPUNIT_ASSERT_EQUAL(string{"Squeeeze"}, string_util::squeeze("Squeeeeeze", 'e'));
  }

  void test_strip() {
    CPPUNIT_ASSERT_EQUAL(string{"Strp"}, string_util::strip("Striip", 'i'));
    CPPUNIT_ASSERT_EQUAL(string{"test\n"}, string_util::strip_trailing_newline("test\n\n"));
  }

  void test_trim() {
    CPPUNIT_ASSERT_EQUAL(string{"testxx"}, string_util::ltrim("xxtestxx", 'x'));
    CPPUNIT_ASSERT_EQUAL(string{"xxtest"}, string_util::rtrim("xxtestxx", 'x'));
    CPPUNIT_ASSERT_EQUAL(string{"test"}, string_util::trim("xxtestxx", 'x'));
  }

  void test_join() {
    CPPUNIT_ASSERT_EQUAL(string{"A, B, C"}, string_util::join({"A", "B", "C"}, ", "));
  }

  void test_split_into() {
    vector<string> strings;
    string_util::split_into("A,B,C", ',', strings);
    CPPUNIT_ASSERT_EQUAL(size_t(3), strings.size());
    CPPUNIT_ASSERT_EQUAL(string{"A"}, strings[0]);
    CPPUNIT_ASSERT_EQUAL(string{"C"}, strings[2]);
  }

  void test_split() {
    vector<string> strings{string{"foo"}, string{"bar"}};
    vector<string> result{string_util::split(string{"foo,bar"}, ',')};
    CPPUNIT_ASSERT(strings.size() == result.size());
    CPPUNIT_ASSERT(strings[0] == result[0]);
    CPPUNIT_ASSERT_EQUAL(string{"bar"}, result[1]);
  }

  void test_find_nth() {
    CPPUNIT_ASSERT_EQUAL(size_t{0}, string_util::find_nth("foobarfoobar", 0, "f", 1));
    CPPUNIT_ASSERT_EQUAL(size_t{6}, string_util::find_nth("foobarfoobar", 0, "f", 2));
    CPPUNIT_ASSERT_EQUAL(size_t{7}, string_util::find_nth("foobarfoobar", 0, "o", 3));
  }

  void test_from_stream() {
    CPPUNIT_ASSERT_EQUAL(string{"zzzfoobar"},
        string_util::from_stream(std::stringstream() << std::setw(6) << std::setfill('z') << "foo"
                                                     << "bar"));
  }

  void test_hash() {
    unsigned long hashA1{string_util::hash("foo")};
    unsigned long hashA2{string_util::hash("foo")};
    unsigned long hashB1{string_util::hash("Foo")};
    unsigned long hashB2{string_util::hash("Bar")};
    CPPUNIT_ASSERT(hashA1 == hashA2);
    CPPUNIT_ASSERT(hashA1 != hashB1 != hashB2);
    CPPUNIT_ASSERT(hashB1 != hashB2);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_string);

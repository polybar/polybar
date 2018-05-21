#include <iomanip>

#include "common/test.hpp"
#include "utils/string.hpp"

using namespace polybar;

TEST(String, upper) {
  EXPECT_EQ("FOO", string_util::upper("FOO"));
  EXPECT_EQ("FOO", string_util::upper("FoO"));
  EXPECT_EQ("FOO", string_util::upper("FOo"));
  EXPECT_EQ("FOO", string_util::upper("Foo"));
}

TEST(String, lower) {
  EXPECT_EQ("bar", string_util::lower("BAR"));
}

TEST(String, compare) {
  EXPECT_TRUE(string_util::compare("foo", "foo"));
  EXPECT_TRUE(string_util::compare("foo", "Foo"));
  EXPECT_FALSE(string_util::compare("foo", "bar"));
}

TEST(String, replace) {
  EXPECT_EQ("a.c", string_util::replace("abc", "b", "."));
  EXPECT_EQ("a.a", string_util::replace("aaa", "a", ".", 1, 2));
  EXPECT_EQ(".aa", string_util::replace("aaa", "a", ".", 0, 2));
  EXPECT_EQ("Foo bxr baz", string_util::replace("Foo bar baz", "a", "x"));
  EXPECT_EQ("foxoobar", string_util::replace("foooobar", "o", "x", 2, 3));
  EXPECT_EQ("foooobar", string_util::replace("foooobar", "o", "x", 0, 1));
}

TEST(String, replaceAll) {
  EXPECT_EQ("Foo bxr bxzx", string_util::replace_all("Foo bar baza", "a", "x"));
  EXPECT_EQ("hoohoohoo", string_util::replace_all("hehehe", "he", "hoo"));
  EXPECT_EQ("hoohehe", string_util::replace_all("hehehe", "he", "hoo", 0, 2));
  EXPECT_EQ("hehehoo", string_util::replace_all("hehehe", "he", "hoo", 4));
  EXPECT_EQ("hehehe", string_util::replace_all("hehehe", "he", "hoo", 0, 1));
  EXPECT_EQ("113113113", string_util::replace_all("131313", "3", "13"));
}

TEST(String, squeeze) {
  EXPECT_EQ("Squeze", string_util::squeeze("Squeeeeeze", 'e'));
  EXPECT_EQ("bar baz foobar", string_util::squeeze("bar  baz   foobar", ' '));
}

TEST(String, strip) {
  EXPECT_EQ("Strp", string_util::strip("Striip", 'i'));
  EXPECT_EQ("test\n", string_util::strip_trailing_newline("test\n\n"));
}

TEST(String, trim) {
  EXPECT_EQ("x x", string_util::trim("  x x "));
  EXPECT_EQ("testxx", string_util::ltrim("xxtestxx", 'x'));
  EXPECT_EQ("xxtest", string_util::rtrim("xxtestxx", 'x'));
  EXPECT_EQ("test", string_util::trim("xxtestxx", 'x'));
}

TEST(String, join) {
  EXPECT_EQ("A, B, C", string_util::join({"A", "B", "C"}, ", "));
}

TEST(String, splitInto) {
  vector<string> strings;
  string_util::split_into("A,B,C", ',', strings);
  EXPECT_EQ(3, strings.size());
  EXPECT_EQ("A", strings[0]);
  EXPECT_EQ("C", strings[2]);
}

TEST(String, split) {
  vector<string> strings{"foo", "bar"};
  vector<string> result{string_util::split("foo,bar", ',')};
  EXPECT_EQ(strings.size(), result.size());
  EXPECT_EQ(strings[0], result[0]);
  EXPECT_EQ("bar", result[1]);
}

TEST(String, findNth) {
  EXPECT_EQ(0, string_util::find_nth("foobarfoobar", 0, "f", 1));
  EXPECT_EQ(6, string_util::find_nth("foobarfoobar", 0, "f", 2));
  EXPECT_EQ(7, string_util::find_nth("foobarfoobar", 0, "o", 3));
}

TEST(String, hash) {
  unsigned long hashA1{string_util::hash("foo")};
  unsigned long hashA2{string_util::hash("foo")};
  unsigned long hashB1{string_util::hash("Foo")};
  unsigned long hashB2{string_util::hash("Bar")};
  EXPECT_EQ(hashA2, hashA1);
  EXPECT_NE(hashB1, hashA1);
  EXPECT_NE(hashB2, hashA1);
  EXPECT_NE(hashB2, hashB1);
}

TEST(String, floatingPoint) {
  EXPECT_EQ("1.26", string_util::floating_point(1.2599, 2));
  EXPECT_EQ("2", string_util::floating_point(1.7, 0));
  EXPECT_EQ("1.7770000000", string_util::floating_point(1.777, 10));
}

TEST(String, filesize) {
  EXPECT_EQ("3.000 MB", string_util::filesize_mb(3 * 1024, 3));
  EXPECT_EQ("3.195 MB", string_util::filesize_mb(3 * 1024 + 200, 3));
  EXPECT_EQ("3 MB", string_util::filesize_mb(3 * 1024 + 400));
  EXPECT_EQ("4 MB", string_util::filesize_mb(3 * 1024 + 800));
  EXPECT_EQ("3.195 GB", string_util::filesize_gb(3 * 1024 * 1024 + 200 * 1024, 3));
  EXPECT_EQ("3 GB", string_util::filesize_gb(3 * 1024 * 1024 + 400 * 1024));
  EXPECT_EQ("4 GB", string_util::filesize_gb(3 * 1024 * 1024 + 800 * 1024));
  EXPECT_EQ("3 B", string_util::filesize(3));
  EXPECT_EQ("3 KB", string_util::filesize(3 * 1024));
  EXPECT_EQ("3 MB", string_util::filesize(3 * 1024 * 1024));
  EXPECT_EQ("3 GB", string_util::filesize((unsigned long long)3 * 1024 * 1024 * 1024));
  EXPECT_EQ("3 TB", string_util::filesize((unsigned long long)3 * 1024 * 1024 * 1024 * 1024));
}

TEST(String, operators) {
  string foo = "foobar";
  EXPECT_EQ("foo", foo - "bar");
  string baz = "bazbaz";
  EXPECT_EQ("bazbaz", baz - "ba");
  EXPECT_EQ("bazbaz", baz - "baZ");
  EXPECT_EQ("bazbaz", baz - "bazbz");
  string aaa = "aaa";
  EXPECT_EQ("aaa", aaa - "aaaaa");
}

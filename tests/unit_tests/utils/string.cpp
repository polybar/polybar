#include "utils/string.hpp"

#include "common/test.hpp"

using namespace polybar;

TEST(String, ends_with) {
  EXPECT_TRUE(string_util::ends_with("foo", "foo"));
  EXPECT_TRUE(string_util::ends_with("foobar", "bar"));
  EXPECT_TRUE(string_util::ends_with("foobar", ""));
  EXPECT_FALSE(string_util::ends_with("foo", "bar"));
  EXPECT_FALSE(string_util::ends_with("foo", "Foo"));
  EXPECT_FALSE(string_util::ends_with("", "Foo"));
}

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

TEST(String, contains) {
  EXPECT_TRUE(string_util::contains("fooooobar", "foo"));
  EXPECT_TRUE(string_util::contains("barrrrrrfoo", "foo"));
  EXPECT_TRUE(string_util::contains("barrfoobazzz", "foo"));
  EXPECT_TRUE(string_util::contains("foo", "foo"));
  EXPECT_TRUE(string_util::contains("foobar", "foo"));
  EXPECT_TRUE(string_util::contains("foobar", "bar"));
  EXPECT_FALSE(string_util::contains("foo", "Foo"));
  EXPECT_FALSE(string_util::contains("foo", "bar"));
  EXPECT_FALSE(string_util::contains("foobar", "baz"));
  EXPECT_FALSE(string_util::contains("foobAr", "bar"));
}

TEST(String, contains_ignore_case) {
  EXPECT_TRUE(string_util::contains_ignore_case("fooooobar", "foo"));
  EXPECT_TRUE(string_util::contains_ignore_case("barrrrrrfoo", "foo"));
  EXPECT_TRUE(string_util::contains_ignore_case("barrfoobazzz", "foo"));
  EXPECT_TRUE(string_util::contains_ignore_case("fooooobar", "fOO"));
  EXPECT_TRUE(string_util::contains_ignore_case("barrrrrrfoo", "FOo"));
  EXPECT_TRUE(string_util::contains_ignore_case("barrfoobazzz", "FoO"));
  EXPECT_TRUE(string_util::contains_ignore_case("foo", "Foo"));
  EXPECT_FALSE(string_util::contains_ignore_case("foo", "bar"));
  EXPECT_TRUE(string_util::contains_ignore_case("foo", ""));
  EXPECT_FALSE(string_util::contains_ignore_case("", "bar"));
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

TEST(String, trimPredicate) {
  EXPECT_EQ("x\t x", string_util::trim("\t  x\t x   ", isspace));
  EXPECT_EQ("x\t x", string_util::trim("x\t x   ", isspace));
}

TEST(String, join) {
  EXPECT_EQ("A, B, C", string_util::join({"A", "B", "C"}, ", "));
}

TEST(String, split) {
  {
    vector<string> strings = string_util::split("A,B,C", ',');
    EXPECT_EQ(3, strings.size());
    EXPECT_EQ("A", strings[0]);
    EXPECT_EQ("B", strings[1]);
    EXPECT_EQ("C", strings[2]);
  }

  {
    vector<string> strings = string_util::split(",A,,B,,C,", ',');
    EXPECT_EQ(3, strings.size());
    EXPECT_EQ("A", strings[0]);
    EXPECT_EQ("B", strings[1]);
    EXPECT_EQ("C", strings[2]);
  }
}

TEST(String, tokenize) {
  {
    vector<string> strings = string_util::tokenize("A,B,C", ',');
    EXPECT_EQ(3, strings.size());
    EXPECT_EQ("A", strings[0]);
    EXPECT_EQ("B", strings[1]);
    EXPECT_EQ("C", strings[2]);
  }

  {
    using namespace std::string_literals;
    vector<string> strings = string_util::tokenize(",A,,B,,C,", ',');
    vector<string> result{""s, "A"s, ""s, "B"s, ""s, "C"s, ""s};

    EXPECT_TRUE(strings == result);
  }
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
  EXPECT_EQ("3.000 MiB", string_util::filesize_mib(3 * 1024, 3));
  EXPECT_EQ("3.195 MiB", string_util::filesize_mib(3 * 1024 + 200, 3));
  EXPECT_EQ("3 MiB", string_util::filesize_mib(3 * 1024 + 400));
  EXPECT_EQ("4 MiB", string_util::filesize_mib(3 * 1024 + 800));
  EXPECT_EQ("3.195 GiB", string_util::filesize_gib(3 * 1024 * 1024 + 200 * 1024, 3));
  EXPECT_EQ("3 GiB", string_util::filesize_gib(3 * 1024 * 1024 + 400 * 1024));
  EXPECT_EQ("4 GiB", string_util::filesize_gib(3 * 1024 * 1024 + 800 * 1024));
  EXPECT_EQ("3 B", string_util::filesize(3));
  EXPECT_EQ("3 KB", string_util::filesize(3 * 1024));
  EXPECT_EQ("3 MB", string_util::filesize(3 * 1024 * 1024));
  EXPECT_EQ("3 GB", string_util::filesize((unsigned long long)3 * 1024 * 1024 * 1024));
  EXPECT_EQ("3 TB", string_util::filesize((unsigned long long)3 * 1024 * 1024 * 1024 * 1024));
}

// utf8_to_ucs4 {{{
class Utf8ToUCS4AsciiTest : public testing::TestWithParam<string> {};

const vector<string> utf8_to_ucs4_ascii_list = {"", "Hello World", "\n", "\0", "\u007f"};

INSTANTIATE_TEST_SUITE_P(Inst, Utf8ToUCS4AsciiTest, testing::ValuesIn(utf8_to_ucs4_ascii_list));

/**
 * Test that the conversion to ucs4 works correctly with pure ASCII strings.
 */
TEST_P(Utf8ToUCS4AsciiTest, correctness) {
  string_util::unicode_charlist result_list{};
  string str = GetParam();

  bool valid = string_util::utf8_to_ucs4(str, result_list);
  ASSERT_TRUE(valid);

  ASSERT_EQ(str.size(), result_list.size());

  for (size_t i = 0; i < str.size(); i++) {
    const auto& unicode_char = result_list[i];
    auto c = str[i];

    // Matches the single byte character
    EXPECT_EQ(c, unicode_char.codepoint);
    // Is at the same offset as in the original string
    EXPECT_EQ(i, unicode_char.offset);
    // Only takes a single byte
    EXPECT_EQ(1, unicode_char.length);
  }
}

// String containing a single codepoint and the expected numerical codepoint
using single_test_t = std::pair<string, uint32_t>;
class Utf8ToUCS4SingleTest : public testing::TestWithParam<single_test_t> {};

const vector<single_test_t> utf8_to_ucs4_single_list = {
    {" ", 0x20},              // Single ASCII character
    {"\u007f", 0x7f},         // End of 1 byte range
    {"\u0080", 0x80},         // Start of 2 byte range
    {"\u07ff", 0x7ff},        // End of 2 byte range
    {"\u0800", 0x800},        // Start of 3 byte range
    {"\uffff", 0xffff},       // End of 3 byte range
    {"\U00010000", 0x10000},  // Start of 4 byte range
    {"\U0010ffff", 0x10ffff}, // End of 4 byte range
    {"\U0001f600", 0x1f600},  // Grinning face emoji
};

INSTANTIATE_TEST_SUITE_P(Inst, Utf8ToUCS4SingleTest, testing::ValuesIn(utf8_to_ucs4_single_list));

/**
 * Test that the conversion to ucs4 works correctly with a single UTF8 character
 */
TEST_P(Utf8ToUCS4SingleTest, correctness) {
  string_util::unicode_charlist result_list{};
  const auto [str, codepoint] = GetParam();

  bool valid = string_util::utf8_to_ucs4(str, result_list);
  ASSERT_TRUE(valid);

  ASSERT_EQ(1, result_list.size());

  auto unicode_char = result_list.front();

  EXPECT_EQ(0, unicode_char.offset);
  // Must encompass entire string
  EXPECT_EQ(str.size(), unicode_char.length);
  // Must match expected codepoint
  EXPECT_EQ(codepoint, unicode_char.codepoint);
}

class Utf8ToUCS4InvalidTest : public testing::TestWithParam<string> {};

const vector<string> utf8_to_ucs4_invalid_list = {
    "\x80",         // continuation byte without leading byte
    "\xa0",         // 2 byte code point with only leading byte
    "\xe0",         // 3 byte code point with only leading byte
    "\xf0",         // 4 byte code point with only leading byte
    "\xf0\x80\x80", // 4 byte code point with only 3 bytes
    "\xe0\xf0\x80", // 3 byte code point, 2nd byte has no continuation prefix
};

INSTANTIATE_TEST_SUITE_P(Inst, Utf8ToUCS4InvalidTest, testing::ValuesIn(utf8_to_ucs4_invalid_list));

/**
 * Tests that the conversion correctly returns false for invalid strings.
 */
TEST_P(Utf8ToUCS4InvalidTest, correctness) {
  string_util::unicode_charlist result_list{};
  const auto str = GetParam();
  bool valid = string_util::utf8_to_ucs4(str, result_list);
  EXPECT_FALSE(valid);
  EXPECT_EQ(0, result_list.size());
}

/**
 * Tests that the conversion works with partially valid strings and that invalid parts are dropped.
 */
TEST(String, utf8ToUCS4Partial) {
  string_util::unicode_charlist result_list{};
  string str = "\xe0\x70\x80"; // a valid ascii character between two invalid characters
  bool valid = string_util::utf8_to_ucs4(str, result_list);
  EXPECT_FALSE(valid);
  EXPECT_EQ(1, result_list.size());

  EXPECT_EQ(0x70, result_list[0].codepoint);
  EXPECT_EQ(1, result_list[0].offset);
  EXPECT_EQ(1, result_list[0].length);
}
// }}}

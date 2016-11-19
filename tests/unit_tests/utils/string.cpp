#include <iomanip>

#include "utils/string.hpp"

int main() {
  using namespace polybar;

  "upper"_test = [] { expect(string_util::upper("FOO") == "FOO"); };

  "lower"_test = [] { expect(string_util::lower("BAR") == "bar"); };

  "compare"_test = [] {
    expect(string_util::compare("foo", "foo"));
    expect(!string_util::compare("foo", "bar"));
  };

  "replace"_test = [] { expect(string_util::replace("Foo bar baz", "a", "x") == "Foo bxr baz"); };

  "replace_all"_test = [] {
    expect(string_util::replace_all("Foo bar baz", "a", "x") == "Foo bxr bxz");
    expect(string_util::replace_all("hehehe", "he", "hoo") == "hoohoohoo");
    expect(string_util::replace_all("131313", "3", "13") == "113113113");
  };

  "squeeze"_test = [] {
    expect(string_util::squeeze("Squeeeeeze", 'e') == "Squeze");
    expect(string_util::squeeze("bar  baz   foobar", ' ') == "bar baz foobar");
  };

  "strip"_test = [] {
    expect(string_util::strip("Striip", 'i') == "Strp");
    expect(string_util::strip_trailing_newline("test\n\n") == "test\n");
  };

  "trim"_test = [] {
    expect(string_util::ltrim("xxtestxx", 'x') == "testxx");
    expect(string_util::rtrim("xxtestxx", 'x') == "xxtest");
    expect(string_util::trim("xxtestxx", 'x') == "test");
  };

  "join"_test = [] { expect(string_util::join({"A", "B", "C"}, ", ") == "A, B, C"); };

  "split_into"_test = [] {
    vector<string> strings;
    string_util::split_into("A,B,C", ',', strings);
    expect(strings.size() == size_t(3));
    expect(strings[0] == "A");
    expect(strings[2] == "C");
  };

  "split"_test = [] {
    vector<string> strings{"foo", "bar"};
    vector<string> result{string_util::split("foo,bar", ',')};
    expect(result.size() == strings.size());
    expect(result[0] == strings[0]);
    expect(result[1] == "bar");
  };

  "find_nth"_test = [] {
    expect(string_util::find_nth("foobarfoobar", 0, "f", 1) == size_t{0});
    expect(string_util::find_nth("foobarfoobar", 0, "f", 2) == size_t{6});
    expect(string_util::find_nth("foobarfoobar", 0, "o", 3) == size_t{7});
  };

  "from_stream"_test = [] {
    auto result =
        string_util::from_stream(std::stringstream() << std::setw(6) << std::setfill('z') << "foo"
                                                     << "bar");
    expect(result == "zzzfoobar");
  };

  "hash"_test = [] {
    unsigned long hashA1{string_util::hash("foo")};
    unsigned long hashA2{string_util::hash("foo")};
    unsigned long hashB1{string_util::hash("Foo")};
    unsigned long hashB2{string_util::hash("Bar")};
    expect(hashA1 == hashA2);
    expect(hashA1 != hashB1 != hashB2);
    expect(hashB1 != hashB2);
  };
}

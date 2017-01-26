#include <iomanip>

#include "utils/string.cpp"

int main() {
  using namespace polybar;

  "upper"_test = [] { expect(string_util::upper("FOO") == "FOO"); };

  "lower"_test = [] { expect(string_util::lower("BAR") == "bar"); };

  "compare"_test = [] {
    expect(string_util::compare("foo", "foo"));
    expect(!string_util::compare("foo", "bar"));
  };

  "replace"_test = [] {
    expect(string_util::replace("abc", "b", ".") == "a.c");
    expect(string_util::replace("aaa", "a", ".", 1, 2) == "a.a");
    expect(string_util::replace("aaa", "a", ".", 0, 2) == ".aa");
    expect(string_util::replace("Foo bar baz", "a", "x") == "Foo bxr baz");
    expect(string_util::replace("foooobar", "o", "x", 2, 3) == "foxoobar");
    expect(string_util::replace("foooobar", "o", "x", 0, 1) == "foooobar");
  };

  "replace_all"_test = [] {
    expect(string_util::replace_all("Foo bar baza", "a", "x") == "Foo bxr bxzx");
    expect(string_util::replace_all("hehehe", "he", "hoo") == "hoohoohoo");
    expect(string_util::replace_all("hehehe", "he", "hoo", 0, 2) == "hoohehe");
    expect(string_util::replace_all("hehehe", "he", "hoo", 4) == "hehehoo");
    expect(string_util::replace_all("hehehe", "he", "hoo", 0, 1) == "hehehe");
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
    expect(string_util::trim("  x x ") == "x x");
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

  "hash"_test = [] {
    unsigned long hashA1{string_util::hash("foo")};
    unsigned long hashA2{string_util::hash("foo")};
    unsigned long hashB1{string_util::hash("Foo")};
    unsigned long hashB2{string_util::hash("Bar")};
    expect(hashA1 == hashA2);
    expect(hashA1 != hashB1);
    expect(hashA1 != hashB2);
    expect(hashB1 != hashB2);
  };

  "floating_point"_test = [] {
    expect(string_util::floating_point(1.2599, 2) == "1.26");
    expect(string_util::floating_point(1.7, 0) == "2");
    expect(string_util::floating_point(1.777, 10) == "1.7770000000");
  };

  "filesize"_test = [] {
    expect(string_util::filesize_mb(3 * 1024, 3) == "3.000 MB");
    expect(string_util::filesize_mb(3 * 1024 + 200, 3) == "3.195 MB");
    expect(string_util::filesize_mb(3 * 1024 + 400) == "3 MB");
    expect(string_util::filesize_mb(3 * 1024 + 800) == "4 MB");
    expect(string_util::filesize_gb(3 * 1024 * 1024 + 200 * 1024, 3) == "3.195 GB");
    expect(string_util::filesize_gb(3 * 1024 * 1024 + 400 * 1024) == "3 GB");
    expect(string_util::filesize_gb(3 * 1024 * 1024 + 800 * 1024) == "4 GB");
    expect(string_util::filesize(3 * 1024 * 1024) == "3 GB");
  };

  "sstream"_test = [] {
    string s;
    expect((s = (sstream() << "test")) == "test"s);
    expect((s = (sstream() << std::setprecision(2) << std::fixed << 1.25)).erase(0, 2) == "25"s);
  };

  "operators"_test = [] {
    string foo = "foobar";
    expect(foo - "bar" == "foo");
    string baz = "bazbaz";
    expect(baz - "ba" == "bazbaz");
    expect(baz - "bazbz" == "bazbaz");
    string aaa = "aaa";
    expect(aaa - "aaaaa" == "aaa");
  };
}

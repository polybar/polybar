#include <iomanip>
#include <iostream>
#include <memory>

#include <libgen.h> // dirname

#include "common/test.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"

using namespace polybar;

namespace {

  auto expand_with_shell(string const& path) -> string {
    auto cmd = command_util::make_command("echo " + path);
    cmd->exec();

    return cmd->readline();
  }

}

TEST(File, expand) {
  struct TestCase {
      string input;
      string expected;
  };

  std::vector<TestCase> test_cases = {
    // '/' is inserted to relative paths
    { "a", "/a" },

    // '.' and '..' are handled
    { "/a/./b", "/a/b" },
    { "/a/b/../c", "/a/c" },
    { "/a/b/./././c", "/a/b/c" },

    // consecutive '/'s are stripped
    { "/a/b////c", "/a/b/c" },

    // trailing '/' is preserved
    { "a/", "/a/" },
    { "/a/./b/", "/a/b/" },

    // expand home directories
    { "~/test", expand_with_shell("~/test") },
    { "./$HOME/test", expand_with_shell("$HOME/test") },

    // handle '.' and '..' correctly
    { "", "/" },
    { "/", "/" },
    { "//", "/" },
    { "/../", "/" },
    { "/../..", "/" },
    { "/..////..", "/" },
    { "/.././.././..", "/" },
    { ".", "/" },

    // only leading tilde is expanded
    { "/tmp/~", "/tmp/~" },
    { "/tmp/~root", "/tmp/~root" },
    { "~/~", expand_with_shell("~/~") },
    { "~/~", expand_with_shell("$HOME/~") },

    // HOME is expanded anywhere
    { "/tmp/$HOME", expand_with_shell("/tmp$HOME")},
    { "/tmp/$HOME", "/tmp" + expand_with_shell("~")},
  };

  for (auto const& test_case : test_cases) {
    const auto expanded = file_util::expand(test_case.input);
    EXPECT_EQ(expanded, test_case.expected) << "input = " << test_case.input;
  }

  // correctly handles POSIX "~username" expansion (use root since every system has a user with that name)
  EXPECT_NE(file_util::expand("~root/test"), "~root/test");
  EXPECT_EQ(file_util::expand("~root/test"), expand_with_shell("~root/test"));

  // expand the current user's home directory
  auto user_home = "~" + expand_with_shell("$USER") + "/test";
  EXPECT_NE(file_util::expand(user_home), user_home);
  EXPECT_EQ(file_util::expand(user_home), expand_with_shell(user_home));
}

TEST(File, expandRelativeTo) {
  struct TestCase {
    string base_dir;
    string path;
    string expected;
  };

  std::vector<TestCase> test_cases = {
    // relative path, absolute base dir
    { "/", "a", "/a" },
    { "/foo", "bar", "/foo/bar" },
    { "//foo///", "bar/", "/foo/bar/" },

    // absolute path (base dir is ignored)
    { "/foo", "/bar", "/bar" },
    { "/a/b/c", "/x/y/z", "/x/y/z" },

    // relative base dir and relative path (result has '/' prepended)
    { "foo", "bar", "/foo/bar" },
    { "a/b/c", "x/y/z", "/a/b/c/x/y/z" },

    // special components "." and ".." are handled in both
    { "/foo", "../bar", "/bar" },
    { "/foo/bar", "../baz", "/foo/baz" },
    { "/foo/../bar", "baz", "/bar/baz" },
    { "/a/./b/./c", "d/./.././e", "/a/b/c/e" },
    { "../foo", "../bar", "/bar" },
  };

  for (auto const& test_case : test_cases) {
    const auto expanded = file_util::expand_relative_to(test_case.path, test_case.base_dir);
    EXPECT_EQ(expanded, test_case.expected)
      << "base_dir = " << test_case.base_dir << ", "
      << "path = " << test_case.path;
  }
}

TEST(File, dirname) {
  struct TestCase {
      string input;
      string expected;
  };

  // all test cases are from dirname(3) manpage
  std::vector<TestCase> test_cases = {
    { "/usr/lib", "/usr" },
    { "/usr/", "/" },
    { "usr", "." },
    { "/", "/" },
    { ".", "." },
    { "..", "." },
    { "", "." },
  };

  for (auto const& test_case : test_cases) {
    const auto dir_name = file_util::dirname(test_case.input);
    EXPECT_EQ(dir_name, test_case.expected) << "input = " << test_case.input;

    auto buffer = std::make_unique<char[]>(test_case.input.length() + 1);

    std::copy(test_case.input.begin(), test_case.input.end(), buffer.get());
    buffer[test_case.input.length()] = '\0';

    auto result = ::dirname(buffer.get());
    ASSERT_NE( result, nullptr );
    ASSERT_STREQ(result, dir_name.c_str());
  }
}

TEST(File, isAbsolute) {
  std::vector<string> absolute_paths = {
    // simple paths
    "/usr/lib",
    "/usr/",
    "/",

    // paths using tilde expansion
    "~",
    "~/",
    "~root",
    "~root/",

    // path with env variable
    "$HOME",
  };

  for (auto const& path : absolute_paths) {
    EXPECT_TRUE(file_util::is_absolute(path)) << "path = " << path;
  }

  std::vector<string> relative_paths = {
    "usr",
    ".",
    "..",
    "",
  };

  for (auto const& path : relative_paths) {
    EXPECT_FALSE(file_util::is_absolute(path)) << "path = " << path;
  }
}

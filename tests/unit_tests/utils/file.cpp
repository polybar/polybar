#include <iomanip>
#include <iostream>

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
  // '/' is inserted to relative paths
  EXPECT_EQ(file_util::expand("a"), "/a");

  // '.' and '..' are handled
  EXPECT_EQ(file_util::expand("/a/./b"), "/a/b");
  EXPECT_EQ(file_util::expand("/a/b/../c"), "/a/c");
  EXPECT_EQ(file_util::expand("/a/b/./././c"), "/a/b/c");

  // consecutive '/'s are stripped
  EXPECT_EQ(file_util::expand("/a/b////c"), "/a/b/c");

  // trailing '/' is preserved
  EXPECT_EQ(file_util::expand("a/"), "/a/");
  EXPECT_EQ(file_util::expand("/a/./b/"), "/a/b/");

  EXPECT_EQ(file_util::expand(""), "/");
  EXPECT_EQ(file_util::expand("/"), "/");
  EXPECT_EQ(file_util::expand("//"), "/");
  EXPECT_EQ(file_util::expand("/../"), "/");
  EXPECT_EQ(file_util::expand("/../.."), "/");
  EXPECT_EQ(file_util::expand("/..////.."), "/");
  EXPECT_EQ(file_util::expand("/.././.././.."), "/");
  EXPECT_EQ(file_util::expand("."), "/");

  // expand home directories
  EXPECT_EQ(file_util::expand("~/test"), expand_with_shell("~/test"));
  EXPECT_EQ(file_util::expand("./$HOME/test"), expand_with_shell("$HOME/test"));

  EXPECT_NE(file_util::expand("~root/test"), "~root/test");
  EXPECT_EQ(file_util::expand("~root/test"), expand_with_shell("~root/test"));

  auto user_home = "~" + expand_with_shell("$USER") + "/test";
  EXPECT_NE(file_util::expand(user_home), user_home);
  EXPECT_EQ(file_util::expand(user_home), expand_with_shell(user_home));

  // only leading tilde is expanded
  EXPECT_EQ(file_util::expand("/tmp/~"), "/tmp/~");
  EXPECT_EQ(file_util::expand("/tmp/~root"), "/tmp/~root");
  EXPECT_EQ(file_util::expand("~/~"), expand_with_shell("~/~"));
  EXPECT_EQ(file_util::expand("~/~"), expand_with_shell("$HOME/~"));

  // HOME is expanded anywhere
  EXPECT_EQ(file_util::expand("/tmp/$HOME"), expand_with_shell("/tmp$HOME"));
  EXPECT_EQ(file_util::expand("/tmp/$HOME"), "/tmp" + expand_with_shell("~"));
}

TEST(File, dirname) {
  // from dirname(3) manpage
  EXPECT_EQ(file_util::dirname("/usr/lib"), "/usr");
  EXPECT_EQ(file_util::dirname("/usr/"), "/");
  EXPECT_EQ(file_util::dirname("usr"), ".");
  EXPECT_EQ(file_util::dirname("/"), "/");
  EXPECT_EQ(file_util::dirname("."), ".");
  EXPECT_EQ(file_util::dirname(".."), ".");
  EXPECT_EQ(file_util::dirname(""), ".");
}

TEST(File, isAbsolute) {
  EXPECT_TRUE(file_util::is_absolute("/usr/lib"));
  EXPECT_TRUE(file_util::is_absolute("/usr/"));
  EXPECT_TRUE(file_util::is_absolute("/"));

  EXPECT_TRUE(file_util::is_absolute("~"));
  EXPECT_TRUE(file_util::is_absolute("~/"));
  EXPECT_TRUE(file_util::is_absolute("~root"));
  EXPECT_TRUE(file_util::is_absolute("~root/"));

  EXPECT_FALSE(file_util::is_absolute("usr"));
  EXPECT_FALSE(file_util::is_absolute("."));
  EXPECT_FALSE(file_util::is_absolute(".."));
  EXPECT_FALSE(file_util::is_absolute(""));
}

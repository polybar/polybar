#include "ipc/util.hpp"

#include "common/test.hpp"
#include "ipc/msg.hpp"

using namespace polybar;
using namespace ipc;

TEST(GetSocketPath, RoundTrip) {
  EXPECT_EQ(123, get_pid_from_socket(get_socket_path(123)));
  EXPECT_EQ(1, get_pid_from_socket(get_socket_path(1)));

  EXPECT_EQ(-1, get_pid_from_socket(get_glob_socket_path()));
}

TEST(PidFromSocket, EdgeCases) {
  EXPECT_EQ(-1, get_pid_from_socket(""));
  EXPECT_EQ(-1, get_pid_from_socket("/tmp/foo.txt"));
  EXPECT_EQ(-1, get_pid_from_socket("/tmp/foo.sock"));
  EXPECT_EQ(-1, get_pid_from_socket("/tmp/foo..sock"));
  EXPECT_EQ(-1, get_pid_from_socket("/tmp/foo.bar.sock"));
}

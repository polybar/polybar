#include "utils/process.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "common/test.hpp"

using namespace polybar;
using namespace process_util;

TEST(SpawnAsync, is_async) {
  pid_t pid = spawn_async([] { exec_sh("sleep 0.1"); });
  int status;

  pid_t res = process_util::wait_for_completion_nohang(pid, &status);

  ASSERT_NE(res, -1);

  EXPECT_FALSE(WIFEXITED(status));
}

TEST(SpawnAsync, exit_code) {
  pid_t pid = spawn_async([] { exec_sh("exit 42"); });
  int status = 0;
  pid_t res = waitpid(pid, &status, 0);

  EXPECT_EQ(res, pid);

  EXPECT_EQ(WEXITSTATUS(status), 42);
}

TEST(SpawnAsync, env) {
  pid_t pid = spawn_async([] { exec_sh("exit $EXIT", {{"EXIT", "45"}}); });
  int status = 0;
  pid_t res = waitpid(pid, &status, 0);

  EXPECT_EQ(res, pid);

  EXPECT_EQ(WEXITSTATUS(status), 45);
}

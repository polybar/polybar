#pragma once

#include <sys/types.h>

#include "common.hpp"

POLYBAR_NS

namespace process_util {
  bool in_parent_process(pid_t pid);
  bool in_forked_process(pid_t pid);

  void redirect_stdio_to_dev_null();

  pid_t spawn_async(std::function<void()> const& lambda);
  void fork_detached(std::function<void()> const& lambda);

  void exec(char* cmd, char** args);
  void exec_sh(const char* cmd, const vector<pair<string, string>>& env = {});

  int wait(pid_t pid);

  pid_t wait_for_completion(pid_t process_id, int* status_addr = nullptr, int waitflags = 0);
  pid_t wait_for_completion(int* status_addr, int waitflags = 0);
  pid_t wait_for_completion_nohang(pid_t process_id, int* status);
  pid_t wait_for_completion_nohang(int* status);
  pid_t wait_for_completion_nohang();

  bool notify_childprocess();
}  // namespace process_util

POLYBAR_NS_END

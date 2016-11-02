#pragma once

#include "common.hpp"

LEMONBUDDY_NS

namespace process_util {
  bool in_parent_process(pid_t pid);
  bool in_forked_process(pid_t pid);

  void exec(string cmd);

  pid_t wait_for_completion(pid_t process_id, int* status_addr, int waitflags = 0);
  pid_t wait_for_completion(int* status_addr, int waitflags = 0);
  pid_t wait_for_completion(pid_t process_id);
  pid_t wait_for_completion_nohang(pid_t process_id, int* status);
  pid_t wait_for_completion_nohang(int* status);
  pid_t wait_for_completion_nohang();

  bool notify_childprocess();

  void unblock_signal(int sig);
}

LEMONBUDDY_NS_END

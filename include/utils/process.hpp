#pragma once

#include "common.hpp"

POLYBAR_NS

namespace process_util {
  bool in_parent_process(pid_t pid);
  bool in_forked_process(pid_t pid);

  void redirect_process_output_to_dev_null();

  void exec(char* cmd, char** args);
  void exec_sh(const char* cmd);

  pid_t wait_for_completion(pid_t process_id, int* status_addr, int waitflags = 0);
  pid_t wait_for_completion(int* status_addr, int waitflags = 0);
  pid_t wait_for_completion(pid_t process_id);
  pid_t wait_for_completion_nohang(pid_t process_id, int* status);
  pid_t wait_for_completion_nohang(int* status);
  pid_t wait_for_completion_nohang();

  bool notify_childprocess();
}

POLYBAR_NS_END

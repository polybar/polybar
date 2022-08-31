#pragma once

#include "common.hpp"

POLYBAR_NS

namespace ipc {
  string get_runtime_path();
  string ensure_runtime_path();
  string get_socket_path(const string& pid_string);
  string get_socket_path(int pid);
  string get_glob_socket_path();
  int get_pid_from_socket(const string& path);
}  // namespace ipc

POLYBAR_NS_END

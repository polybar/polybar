#include "ipc/util.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace ipc {

  static constexpr auto SUFFIX = ".sock";
  static constexpr auto XDG_RUNTIME_DIR = "XDG_RUNTIME_DIR";

  string get_runtime_path() {
    if (env_util::has(XDG_RUNTIME_DIR)) {
      return env_util::get("XDG_RUNTIME_DIR") + "/polybar";
    } else {
      return "/tmp/polybar-" + to_string(getuid());
    }
    return env_util::get("XDG_RUNTIME_DIR", "/tmp") + "/polybar";
  }

  string ensure_runtime_path() {
    string runtime_path = get_runtime_path();
    if (!file_util::exists(runtime_path) && mkdir(runtime_path.c_str(), 0700) == -1) {
      // It's possible the folder was created in the meantime, we have to check again.
      if (!file_util::exists(runtime_path)) {
        throw system_error("Failed to create ipc socket folders");
      }
    }

    return runtime_path;
  }

  string get_socket_path(const string& pid_string) {
    return get_runtime_path() + "/ipc." + pid_string + SUFFIX;
  }

  string get_socket_path(int pid) {
    return get_socket_path(to_string(pid));
  }

  string get_glob_socket_path() {
    return get_socket_path("*");
  }

  int get_pid_from_socket(const string& path) {
    if (!string_util::ends_with(path, SUFFIX)) {
      return -1;
    }

    auto stripped = path.substr(0, path.size() - strlen(SUFFIX));
    auto p = stripped.rfind('.');

    if (p == string::npos) {
      return -1;
    }

    try {
      return std::stoi(stripped.substr(p + 1));
    } catch (...) {
      return -1;
    }
  }
} // namespace ipc

POLYBAR_NS_END

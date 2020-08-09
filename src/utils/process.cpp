#include "utils/process.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace process_util {
  /**
   * Check if currently in main process
   */
  bool in_parent_process(pid_t pid) {
    return pid != -1 && pid != 0;
  }

  /**
   * Check if currently in subprocess
   */
  bool in_forked_process(pid_t pid) {
    return pid == 0;
  }

  void redirect_process_output_to_dev_null() {
    auto redirect = [](int fd_to_redirect) {
      int fd = open("/dev/null", O_WRONLY);
      if (fd < 0 || dup2(fd, fd_to_redirect) < 0) {
        throw system_error("Failed to redirect process output");
      }
      close(fd);
    };

    redirect(STDOUT_FILENO);
    redirect(STDERR_FILENO);
  }

  /**
   * Execute command
   */
  void exec(char* cmd, char** args) {
    if (cmd != nullptr) {
      execvp(cmd, args);
      throw system_error("execvp() failed");
    }
  }

  /**
   * Execute command using shell
   */
  void exec_sh(const char* cmd) {
    if (cmd != nullptr) {
      static const string shell{env_util::get("POLYBAR_SHELL", "/bin/sh")};
      execlp(shell.c_str(), shell.c_str(), "-c", cmd, nullptr);
      throw system_error("execlp() failed");
    }
  }

  /**
   * Wait for child process
   */
  pid_t wait_for_completion(pid_t process_id, int* status_addr, int waitflags) {
    int saved_errno = errno;
    auto retval = waitpid(process_id, status_addr, waitflags);
    errno = saved_errno;
    return retval;
  }

  /**
   * Wait for child process
   */
  pid_t wait_for_completion(int* status_addr, int waitflags) {
    return wait_for_completion(-1, status_addr, waitflags);
  }

  /**
   * Wait for child process
   */
  pid_t wait_for_completion(pid_t process_id) {
    int status = 0;
    return wait_for_completion(process_id, &status);
  }

  /**
   * Non-blocking wait
   *
   * \see wait_for_completion
   */
  pid_t wait_for_completion_nohang(pid_t process_id, int* status) {
    return wait_for_completion(process_id, status, WNOHANG);
  }

  /**
   * Non-blocking wait
   *
   * \see wait_for_completion
   */
  pid_t wait_for_completion_nohang(int* status) {
    return wait_for_completion_nohang(-1, status);
  }

  /**
   * Non-blocking wait
   *
   * \see wait_for_completion
   */
  pid_t wait_for_completion_nohang() {
    int status = 0;
    return wait_for_completion_nohang(-1, &status);
  }

  /**
   * Non-blocking wait call which returns pid of any child process
   *
   * \see wait_for_completion
   */
  bool notify_childprocess() {
    return wait_for_completion_nohang() > 0;
  }
}  // namespace process_util

POLYBAR_NS_END

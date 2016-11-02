#include <sys/wait.h>

#include "utils/process.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

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

  /**
   * Replace process with command
   */
  void exec(string cmd) {
    vector<char*> c_args;
    vector<string> args;

    if (string_util::contains(cmd, "\n"))
      string_util::split_into(cmd, '\n', args);
    else
      string_util::split_into(cmd, ' ', args);

    for (auto&& argument : args) c_args.emplace_back(const_cast<char*>(argument.c_str()));
    c_args.emplace_back(nullptr);

    execvp(c_args[0], c_args.data());

    throw system_error("Failed to execute command");
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
   * @see wait_for_completion
   */
  pid_t wait_for_completion_nohang(pid_t process_id, int* status) {
    return wait_for_completion(process_id, status, WNOHANG);
  }

  /**
   * Non-blocking wait
   *
   * @see wait_for_completion
   */
  pid_t wait_for_completion_nohang(int* status) {
    return wait_for_completion_nohang(-1, status);
  }

  /**
   * Non-blocking wait
   *
   * @see wait_for_completion
   */
  pid_t wait_for_completion_nohang() {
    int status = 0;
    return wait_for_completion_nohang(-1, &status);
  }

  /**
   * Non-blocking wait call which returns pid of any child process
   *
   * @see wait_for_completion
   */
  bool notify_childprocess() {
    return wait_for_completion_nohang() > 0;
  }

  void unblock_signal(int sig) {
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, sig);
    if (pthread_sigmask(SIG_UNBLOCK, &sigmask, nullptr) == -1)
      throw system_error("Failed to change pthread_sigmask");
  }
}

LEMONBUDDY_NS_END

#include "utils/process.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
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

  /**
   * Redirects all io fds (stdin, stdout, stderr) of the current process to /dev/null.
   */
  void redirect_stdio_to_dev_null() {
    auto redirect = [](int fd_to_redirect) {
      int fd = open("/dev/null", O_WRONLY);
      if (fd < 0 || dup2(fd, fd_to_redirect) < 0) {
        throw system_error("Failed to redirect process output");
      }
      close(fd);
    };

    redirect(STDIN_FILENO);
    redirect(STDOUT_FILENO);
    redirect(STDERR_FILENO);
  }

  /**
   * Forks a child process and executes the given lambda function in it.
   *
   * Processes spawned this way need to be waited on by the caller.
   */
  pid_t spawn_async(std::function<void()> const& lambda) {
    pid_t pid = fork();
    switch (pid) {
      case -1:
        throw runtime_error("fork_detached: Unable to fork: " + string(strerror(errno)));
      case 0:
        // Child
        setsid();
        umask(0);
        redirect_stdio_to_dev_null();
        lambda();
        _Exit(0);
        break;
      default:
        return pid;
    }
  }

  /**
   * Forks a child process and completely detaches it.
   *
   * In the child process, the given lambda function is executed.
   * We fork twice so that the first forked process can exit and it's child is
   * reparented to the init process.
   *
   * Ref: https://web.archive.org/web/20120914180018/http://www.steve.org.uk/Reference/Unix/faq_2.html#SEC16
   *
   * Use this if you want to run a command and just forget about it.
   *
   * @returns The PID of the child process
   */
  void fork_detached(std::function<void()> const& lambda) {
    pid_t pid = fork();
    switch (pid) {
      case -1:
        throw runtime_error("fork_detached: Unable to fork: " + string(strerror(errno)));
      case 0:
        // Child
        setsid();

        pid = fork();
        switch (pid) {
          case -1:
            throw runtime_error("fork_detached: Unable to fork: " + string(strerror(errno)));
          case 0:
            // Child
            umask(0);
            redirect_stdio_to_dev_null();
            lambda();
            _Exit(0);
        }

        _Exit(0);
      default:
        /*
         * The first fork immediately exits and we have to collect its exit
         * status
         */
        wait(pid);
    }
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
  void exec_sh(const char* cmd, const vector<pair<string, string>>& env) {
    if (cmd != nullptr) {
      static const string shell{env_util::get("POLYBAR_SHELL", "/bin/sh")};

      for (const auto& kv_pair : env) {
        setenv(kv_pair.first.data(), kv_pair.second.data(), 1);
      }

      execlp(shell.c_str(), shell.c_str(), "-c", cmd, nullptr);
      throw system_error("execlp() failed");
    }
  }

  int wait(pid_t pid) {
    int forkstatus;
    do {
      process_util::wait_for_completion(pid, &forkstatus, WCONTINUED | WUNTRACED);
    } while (!WIFEXITED(forkstatus) && !WIFSIGNALED(forkstatus));

    return WEXITSTATUS(forkstatus);
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
}  // namespace process_util

POLYBAR_NS_END

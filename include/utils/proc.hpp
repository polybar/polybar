#ifndef _UTILS_PROC_HPP_
#define _UTILS_PROC_HPP_

#include <string>
#include <errno.h>
#include <cstring>
#include <signal.h>

#include "exception.hpp"

#define PIPE_READ  0
#define PIPE_WRITE 1

#define STRERRNO std::string(std::strerror(errno))
#define STRERRNO_VAL(s) std::string(std::strerror(s))

#define SIGCSTR(sig) strsignal(sig)
#define SIGNSTR(sig) std::to_string(sig)
#define SIGSTR(sig) std::string(SIGCSTR(sig))

namespace proc
{
  class ExecFailure : public Exception {
    using Exception::Exception;
  };

  pid_t get_process_id();
  pid_t get_parent_process_id();

  bool in_parent_process(pid_t pid);
  bool in_forked_process(pid_t pid);

  pid_t fork();
  bool pipe(int fds[2]);
  void exec(const std::string& cmd) throw(ExecFailure);

  bool kill(pid_t pid, int sig = SIGTERM);

  pid_t wait(int *status);
  pid_t wait_for_completion(pid_t pid, int *status, int options = 0);
  pid_t wait_for_completion(int *status, int options = 0);
  pid_t wait_for_completion_nohang(pid_t pid, int *status);
  pid_t wait_for_completion_nohang(int *status);
  pid_t wait_for_completion_nohang();
}

#endif

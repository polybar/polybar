#include <string>
#include <unistd.h>
#include <sys/wait.h>

#include "services/logger.hpp"
#include "utils/proc.hpp"
#include "utils/string.hpp"

namespace proc
{
  pid_t get_process_id() {
    return getpid();
  }

  pid_t get_parent_process_id() {
    return getppid();
  }

  bool in_parent_process(pid_t pid) {
    return pid != -1 && pid != 0;
  }

  bool in_forked_process(pid_t pid) {
    return pid == 0;
  }

  pid_t fork()
  {
    pid_t pid = ::fork();

    if (pid < 0) {
      log_error("Failed to fork process: "+ STRERRNO +" ("+ std::to_string(errno) +")");
      return -1;
    }

    if (in_parent_process(pid))
      log_trace("Forked process (pid: "+ std::to_string(pid) +")");

    return pid;
  }

  bool pipe(int fds[2])
  {
    if (::pipe(fds) != 0) {
      log_error("Failed to allocate memory for pipe channels");
      return false;
    }

    log_trace("Successfully created pipe channels");
    return true;
  }

  void exec(const std::string& cmd) throw(ExecFailure)
  {
    // log_trace(string::replace_all(cmd, "\n", " "));

    std::vector<char *> c_args;
    std::vector<std::string> args;
    auto pos = cmd.find('\n');

    if (pos != std::string::npos)
      string::split_into(cmd, '\n', args);
    else
      string::split_into(cmd, ' ', args);

    for (auto const &a : args)
      c_args.emplace_back(const_cast<char *>(a.c_str()));
    c_args.emplace_back(nullptr);

    // get_logger()->debug("Executing: \""+ string::join(args, " ") +"\"");

    ::execvp(c_args[0], c_args.data());

    throw ExecFailure("Failed to execute command:\n-> "+ STRERRNO+ " ("+ std::to_string(errno) + ")");
  }

  bool kill(pid_t pid, int sig) {
    return ::kill(pid, sig) != -1;
  }

  pid_t wait(int *status) {
    return ::wait(&status);
  }

  pid_t wait_for_completion(pid_t pid, int *status, int options)
  {
    int saved_errno = errno;
    pid_t retval = waitpid(pid, status, options);
    errno = saved_errno;
    return retval;
  }

  pid_t wait_for_completion(int *status, int options) {
    return wait_for_completion(-1, status, options);
  }

  pid_t wait_for_completion_nohang(pid_t pid, int *status) {
    return wait_for_completion(pid, status, WNOHANG);
  }

  pid_t wait_for_completion_nohang(int *status) {
    return wait_for_completion_nohang(-1, status);
  }

  pid_t wait_for_completion_nohang() {
    int status;
    return wait_for_completion_nohang(-1, &status);
  }
}

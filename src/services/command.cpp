#include <errno.h>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "lemonbuddy.hpp"
#include "services/command.hpp"
#include "services/logger.hpp"
#include "utils/io.hpp"
#include "utils/macros.hpp"
#include "utils/string.hpp"

/**
 * auto cmd = std::make_unique<Command>("cat /etc/rc.local");
 * cmd->exec();
 * cmd->tail(callback); //---> the contents of /etc/rc.local is sent to callback()
 *
 * auto cmd = std::make_unique<Command>("/bin/sh\n-c\n while read -r line; do echo data from parent process: $line; done");
 * cmd->exec();
 * cmd->writeline("Test");
 * std::cout << cmd->readline(); //---> data from parent process: Test
 *
 * auto cmd = std::make_unique<Command>("/bin/sh\n-c\nfor i in 1 2 3; do echo $i; done");
 * std::cout << cmd->readline(); //---> 1
 * std::cout << cmd->readline() << cmd->readline(); //---> 23
 */
Command::Command(const std::string& cmd, int stdout[2], int stdin[2])
  : cmd(cmd)
{
  if (stdin != nullptr) {
    this->stdin[PIPE_READ] = stdin[PIPE_READ];
    this->stdin[PIPE_WRITE] = stdin[PIPE_WRITE];
  } else if (false == proc::pipe(this->stdin))
    throw CommandException("Failed to allocate pipe");

  if (stdout != nullptr) {
    this->stdout[PIPE_READ] = stdout[PIPE_READ];
    this->stdout[PIPE_WRITE] = stdout[PIPE_WRITE];
  } else if (false == proc::pipe(this->stdout)) {
    if ((this->stdin[PIPE_READ] = close(this->stdin[PIPE_READ])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    if ((this->stdin[PIPE_WRITE] = close(this->stdin[PIPE_WRITE])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    throw CommandException("Failed to allocate pipe");
  }
}

Command::~Command()
{
  if (this->stdin[PIPE_READ] > 0 && (close(this->stdin[PIPE_READ]) == -1))
    log_error("Failed to close fd: "+ StrErrno());
  if (this->stdin[PIPE_WRITE] > 0 && (close(this->stdin[PIPE_WRITE]) == -1))
    log_error("Failed to close fd: "+ StrErrno());
  if (this->stdout[PIPE_READ] > 0 && (close(this->stdout[PIPE_READ]) == -1))
    log_error("Failed to close fd: "+ StrErrno());
  if (this->stdout[PIPE_WRITE] > 0 && (close(this->stdout[PIPE_WRITE]) == -1))
    log_error("Failed to close fd: "+ StrErrno());
}

int Command::exec(bool wait_for_completion)
{
  if ((this->fork_pid = proc::fork()) == -1)
    throw CommandException("Failed to fork process: "+ StrErrno());

  if (proc::in_forked_process(this->fork_pid)) {
    if (dup2(this->stdin[PIPE_READ], STDIN_FILENO) == -1)
      throw CommandException("Failed to redirect stdin in child process");
    if (dup2(this->stdout[PIPE_WRITE], STDOUT_FILENO) == -1)
      throw CommandException("Failed to redirect stdout in child process");
    if (dup2(this->stdout[PIPE_WRITE], STDERR_FILENO) == -1)
      throw CommandException("Failed to redirect stderr in child process");

    // Close file descriptors that won't be used by forked process
    if ((this->stdin[PIPE_READ] = close(this->stdin[PIPE_READ])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    if ((this->stdin[PIPE_WRITE] = close(this->stdin[PIPE_WRITE])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    if ((this->stdout[PIPE_READ] = close(this->stdout[PIPE_READ])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    if ((this->stdout[PIPE_WRITE] = close(this->stdout[PIPE_WRITE])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());

    // Replace the forked process with the given command
    proc::exec(cmd);
    std::exit(0);
  } else {
    register_pid(this->fork_pid);

    // Close file descriptors that won't be used by parent process
    if ((this->stdin[PIPE_READ] = close(this->stdin[PIPE_READ])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());
    if ((this->stdout[PIPE_WRITE] = close(this->stdout[PIPE_WRITE])) == -1)
      throw CommandException("Failed to close fd: "+ StrErrno());

    if (wait_for_completion)
      return this->wait();
  }

  return EXIT_SUCCESS;
}

int Command::wait()
{
  // Wait for the child processs to finish
  do {
    pid_t pid;
    char msg[64];

    if ((pid = proc::wait_for_completion(this->fork_pid, &this->fork_status, WCONTINUED | WUNTRACED)) == -1) {
      unregister_pid(this->fork_pid);
      throw CommandException("Process did not finish successfully ("+ IntToStr(this->fork_status) +")");
    }

    if (WIFEXITED(this->fork_status))
      sprintf(msg, "exited with status %d", WEXITSTATUS(this->fork_status));
    else if (WIFSIGNALED(this->fork_status))
      sprintf(msg, "got killed by signal %d (%s)", WTERMSIG(this->fork_status), StrSignalC(WTERMSIG(this->fork_status)));
    else if (WIFSTOPPED(this->fork_status))
      sprintf(msg, "stopped by signal %d (%s)", WSTOPSIG(this->fork_status), StrSignalC(WSTOPSIG(this->fork_status)));
    else if (WIFCONTINUED(this->fork_status) == true)
      sprintf(msg, "continued");

    get_logger()->debug("Command "+ ToStr(msg));
  } while (!WIFEXITED(this->fork_status) && !WIFSIGNALED(this->fork_status));

  unregister_pid(this->fork_pid);

  return this->fork_status;
}

void Command::tail(std::function<void(std::string)> callback) {
  io::tail(this->stdout[PIPE_READ], callback);
}

int Command::writeline(const std::string& data) {
  return io::writeline(this->stdin[PIPE_WRITE], data);
}

int Command::get_stdout(int c) {
  return this->stdout[c];
}

// int Command::get_stdin(int c) {
//   return this->stdin[c];
// }

pid_t Command::get_pid() {
  return this->fork_pid;
}

bool Command::is_running() {
  return proc::wait_for_completion_nohang(this->fork_pid, &this->fork_status) > -1;
}

int Command::get_exit_status() {
  return this->fork_status;
}

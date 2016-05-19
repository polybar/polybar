#ifndef _SERVICES_COMMAND_HPP_
#define _SERVICES_COMMAND_HPP_

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "exception.hpp"
#include "utils/proc.hpp"

class CommandException : public Exception {
  using Exception::Exception;
};

class Command
{
  protected:
    std::string cmd;

    int stdout[2];
    int stdin[2];

    pid_t fork_pid;
    int fork_status;

  public:
    Command(const std::string& cmd, int stdout[2] = nullptr, int stdin[2] = nullptr)
      throw(CommandException);
    ~Command() throw(CommandException);

    int exec(bool wait_for_completion = true) throw(CommandException);
    int wait() throw(CommandException);

    void tail(std::function<void(std::string)> callback);
    int writeline(const std::string& data);

    int get_stdout(int);
    int get_stdin(int);

    pid_t get_pid();
    int get_exit_status();
};

#endif

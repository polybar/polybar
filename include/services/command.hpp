#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "exception.hpp"
#include "utils/proc.hpp"

DefineBaseException(CommandException);

class Command
{
  protected:
    std::string cmd;

    int fd_stdout[2];
    int fd_stdin[2];

    pid_t fork_pid;
    int fork_status;

  public:
    Command(std::string cmd, int stdout[2] = nullptr, int stdin[2] = nullptr);
    ~Command();

    int exec(bool wait_for_completion = true);
    int wait();

    void tail(std::function<void(std::string)> callback);
    int writeline(std::string data);

    int get_stdout(int);
    // int get_stdin(int);

    pid_t get_pid();
    bool is_running();
    int get_exit_status();
};

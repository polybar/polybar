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

    int stdout[2];
    int stdin[2];

    pid_t fork_pid;
    int fork_status;

  public:
    Command(const std::string& cmd, int stdout[2] = nullptr, int stdin[2] = nullptr);
    ~Command();

    int exec(bool wait_for_completion = true);
    int wait();

    void tail(std::function<void(std::string)> callback);
    int writeline(const std::string& data);

    int get_stdout(int);
    // int get_stdin(int);

    // pid_t get_pid();
    // int get_exit_status();
};

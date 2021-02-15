#pragma once

#include <mutex>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "errors.hpp"
#include "utils/factory.hpp"
#include "utils/functional.hpp"

POLYBAR_NS

DEFINE_ERROR(command_error);

/**
 * Wrapper used to execute command in a subprocess.
 * In-/output streams are opened to enable ipc.
 * If the command is created using command_util::make_command<output_policy::REDIRECTED>, the child streams are
 * redirected and you can read the program output or write into the program input.
 *
 * If the command is created using command_util::make_command<output_policy::IGNORED>, the output is not redirected and
 * you can't communicate with the child program.
 *
 * Example usage:
 *
 * \code cpp
 *   auto cmd = command_util::make_command<output_policy::REDIRECTED>("cat /etc/rc.local");
 *   cmd->exec();
 *   cmd->tail([](string s) { std::cout << s << std::endl; });
 * \endcode
 *
 * \code cpp
 *   auto cmd = command_util::make_command<output_policy::REDIRECTED>(
 *    "while read -r line; do echo data from parent process: $line; done");
 *   cmd->exec(false);
 *   cmd->writeline("Test");
 *   cout << cmd->readline();
 *   cmd->wait();
 * \endcode
 *
 * \code cpp
 *   auto cmd = command_util::make_command<output_policy::REDIRECTED>("for i in 1 2 3; do echo $i; done");
 *   cmd->exec();
 *   cout << cmd->readline(); // 1
 *   cout << cmd->readline() << cmd->readline(); // 23
 * \endcode
 *
 * \code cpp
 *   auto cmd = command_util::make_command<output_policy::IGNORED>("ping kernel.org");
 *   int status = cmd->exec();
 * \endcode
 */
template <output_policy>
class command;

template <>
class command<output_policy::IGNORED> {
 public:
  explicit command(const logger& logger, string cmd);
  command(const command&) = delete;
  ~command();

  command& operator=(const command&) = delete;

  int exec(bool wait_for_completion = true);
  void terminate();
  bool is_running();
  int wait();

  pid_t get_pid();
  int get_exit_status();

 protected:
  const logger& m_log;

  string m_cmd;

  pid_t m_forkpid{};
  int m_forkstatus = - 1;
};

template <>
class command<output_policy::REDIRECTED> : private command<output_policy::IGNORED> {
 public:
  explicit command(const logger& logger, string cmd);
  command(const command&) = delete;
  ~command();

  command& operator=(const command&) = delete;

  int exec(bool wait_for_completion = true);
  using command<output_policy::IGNORED>::terminate;
  using command<output_policy::IGNORED>::is_running;
  using command<output_policy::IGNORED>::wait;

  using command<output_policy::IGNORED>::get_pid;
  using command<output_policy::IGNORED>::get_exit_status;

  void tail(callback<string> cb);
  int writeline(string data);
  string readline();

  int get_stdout(int c);
  int get_stdin(int c);

 protected:
  int m_stdout[2]{};
  int m_stdin[2]{};

  std::mutex m_pipelock{};
};

namespace command_util {
  template <output_policy OutputType, typename... Args>
  unique_ptr<command<OutputType>> make_command(Args&&... args) {
    return factory_util::unique<command<OutputType>>(logger::make(), forward<Args>(args)...);
  }
}  // namespace command_util

POLYBAR_NS_END

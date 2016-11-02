#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/threading.hpp"

LEMONBUDDY_NS

namespace command_util {
  DEFINE_ERROR(command_error);
  DEFINE_CHILD_ERROR(command_strerror, command_error);

  /**
   * Wrapper used to execute command in a subprocess.
   * In-/output streams are opened to enable ipc.
   *
   * Example usage:
   *
   * @code cpp
   *   auto cmd = command_util::make_command("cat /etc/rc.local");
   *   cmd->exec();
   *   cmd->tail([](string s) { std::cout << s << std::endl; });
   * @endcode
   *
   * @code cpp
   *   auto cmd = command_util::make_command(
   *    "while read -r line; do echo data from parent process: $line; done");
   *   cmd->exec(false);
   *   cmd->writeline("Test");
   *   cout << cmd->readline();
   *   cmd->wait();
   * @endcode
   *
   * @code cpp
   *   auto cmd = command_util::make_command("for i in 1 2 3; do echo $i; done");
   *   cmd->exec();
   *   cout << cmd->readline(); // 1
   *   cout << cmd->readline() << cmd->readline(); // 23
   * @endcode
   */
  class command {
   public:
    explicit command(const logger& logger, string cmd);

    ~command();

    int exec(bool wait_for_completion = true);
    void terminate();
    bool is_running();
    int wait();

    void tail(callback<string> callback);
    int writeline(string data);
    string readline();

    int get_stdout(int c);
    int get_stdin(int c);
    pid_t get_pid();
    int get_exit_status();

   protected:
    const logger& m_log;

    string m_cmd;

    int m_stdout[2];
    int m_stdin[2];

    pid_t m_forkpid;
    int m_forkstatus;

    threading_util::spin_lock m_pipelock;
  };

  using command_t = unique_ptr<command>;

  template <typename... Args>
  command_t make_command(Args&&... args) {
    return make_unique<command>(configure_logger().create<const logger&>(), forward<Args>(args)...);
  }
}

LEMONBUDDY_NS_END

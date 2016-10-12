#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/io.hpp"
#include "utils/process.hpp"
#include "utils/threading.hpp"

LEMONBUDDY_NS

namespace command_util {
  /**
   * Example usage:
   * @code cpp
   *   auto cmd = make_unique<Command>("cat /etc/rc.local");
   *   cmd->exec();
   *   cmd->tail(callback); //---> the contents of /etc/rc.local is sent to callback()
   *
   *   auto cmd = make_unique<Command>(
   *    "/bin/sh\n-c\n while read -r line; do echo data from parent process: $line; done");
   *   cmd->exec();
   *   cmd->writeline("Test");
   *   cout << cmd->readline(); //---> data from parent process: Test
   *
   *   auto cmd = make_unique<Command>("/bin/sh\n-c\nfor i in 1 2 3; do echo $i; done");
   *   cout << cmd->readline(); //---> 1
   *   cout << cmd->readline() << cmd->readline(); //---> 23
   * @encode
   */
  class command {
   public:
    explicit command(const logger& logger, string cmd, int out[2] = nullptr, int in[2] = nullptr)
        : m_log(logger), m_cmd(cmd) {
      if (in != nullptr) {
        m_stdin[PIPE_READ] = in[PIPE_READ];
        m_stdin[PIPE_WRITE] = in[PIPE_WRITE];
      } else if (pipe(m_stdin) != 0) {
        throw system_error("Failed to allocate pipe");
      }

      if (out != nullptr) {
        m_stdout[PIPE_READ] = out[PIPE_READ];
        m_stdout[PIPE_WRITE] = out[PIPE_WRITE];
      } else if (pipe(m_stdout) != 0) {
        close(m_stdin[PIPE_READ]);
        close(m_stdin[PIPE_WRITE]);
        throw system_error("Failed to allocate pipe");
      }
    }

    ~command() {
      if (is_running()) {
        m_log.warn("command: Sending SIGKILL to forked process (%d)", m_forkpid);
        kill(m_forkpid, SIGKILL);
      }
      if (m_stdin[PIPE_READ] > 0 && (close(m_stdin[PIPE_READ]) == -1))
        m_log.err("command: Failed to close fd: %s (%d)", strerror(errno), errno);
      if (m_stdin[PIPE_WRITE] > 0 && (close(m_stdin[PIPE_WRITE]) == -1))
        m_log.err("command: Failed to close fd: %s (%d)", strerror(errno), errno);
      if (m_stdout[PIPE_READ] > 0 && (close(m_stdout[PIPE_READ]) == -1))
        m_log.err("command: Failed to close fd: %s (%d)", strerror(errno), errno);
      if (m_stdout[PIPE_WRITE] > 0 && (close(m_stdout[PIPE_WRITE]) == -1))
        m_log.err("command: Failed to close fd: %s (%d)", strerror(errno), errno);
    }

    /**
     * Execute the command
     */
    int exec(bool wait_for_completion = true) {
      if ((m_forkpid = fork()) == -1)
        throw system_error("Failed to fork process");

      if (process_util::in_forked_process(m_forkpid)) {
        if (dup2(m_stdin[PIPE_READ], STDIN_FILENO) == -1)
          throw system_error("Failed to redirect stdin in child process");
        if (dup2(m_stdout[PIPE_WRITE], STDOUT_FILENO) == -1)
          throw system_error("Failed to redirect stdout in child process");
        if (dup2(m_stdout[PIPE_WRITE], STDERR_FILENO) == -1)
          throw system_error("Failed to redirect stderr in child process");

        // Close file descriptors that won't be used by forked process
        if ((m_stdin[PIPE_READ] = close(m_stdin[PIPE_READ])) == -1)
          throw system_error("Failed to close fd");
        if ((m_stdin[PIPE_WRITE] = close(m_stdin[PIPE_WRITE])) == -1)
          throw system_error("Failed to close fd");
        if ((m_stdout[PIPE_READ] = close(m_stdout[PIPE_READ])) == -1)
          throw system_error("Failed to close fd");
        if ((m_stdout[PIPE_WRITE] = close(m_stdout[PIPE_WRITE])) == -1)
          throw system_error("Failed to close fd");

        // Replace the forked process with the given command
        process_util::exec(m_cmd);
        std::exit(0);
      } else {
        // Close file descriptors that won't be used by parent process
        if ((m_stdin[PIPE_READ] = close(m_stdin[PIPE_READ])) == -1)
          throw system_error("Failed to close fd");
        if ((m_stdout[PIPE_WRITE] = close(m_stdout[PIPE_WRITE])) == -1)
          throw system_error("Failed to close fd");

        if (wait_for_completion)
          return wait();
      }

      return EXIT_SUCCESS;
    }

    /**
     * Wait for the child processs to finish
     */
    int wait() {
      do {
        pid_t pid;

        if ((pid = process_util::wait_for_completion(
                 m_forkpid, &m_forkstatus, WCONTINUED | WUNTRACED)) == -1) {
          throw system_error(
              "Process did not finish successfully (" + to_string(m_forkstatus) + ")");
        }

        if (WIFEXITED(m_forkstatus))
          m_log.trace("command: Exited with status %d", WEXITSTATUS(m_forkstatus));
        else if (WIFSIGNALED(m_forkstatus))
          m_log.trace("command: Got killed by signal %d (%s)", WTERMSIG(m_forkstatus),
              strerror(WTERMSIG(m_forkstatus)));
        else if (WIFSTOPPED(m_forkstatus))
          m_log.trace("command: Stopped by signal %d (%s)", WSTOPSIG(m_forkstatus),
              strerror(WSTOPSIG(m_forkstatus)));
        else if (WIFCONTINUED(m_forkstatus) == true)
          m_log.trace("command: Continued");
      } while (!WIFEXITED(m_forkstatus) && !WIFSIGNALED(m_forkstatus));

      return m_forkstatus;
    }

    /**
     * Tail command output
     */
    void tail(function<void(string)> callback) {
      io_util::tail(m_stdout[PIPE_READ], callback);
    }

    /**
     * Write line to command input channel
     */
    int writeline(string data) {
      std::lock_guard<threading_util::spin_lock> lck(m_pipelock);
      return io_util::writeline(m_stdin[PIPE_WRITE], data);
    }

    /**
     * Get command output channel
     */
    int get_stdout(int c) {
      return m_stdout[c];
    }

    /**
     * Get command input channel
     */
    int get_stdin(int c) {
      return m_stdin[c];
    }

    /**
     * Get command pid
     */
    pid_t get_pid() {
      return m_forkpid;
    }

    /**
     * Check if command is running
     */
    bool is_running() {
      return process_util::wait_for_completion_nohang(m_forkpid, &m_forkstatus) > -1;
    }

    /**
     * Get command exit status
     */
    int get_exit_status() {
      return m_forkstatus;
    }

   protected:
    const logger& m_log;

    string m_cmd;

    int m_stdout[2];
    int m_stdin[2];

    pid_t m_forkpid;
    int m_forkstatus;

    threading_util::spin_lock m_pipelock;
  };

  template <typename... Args>
  auto make_command(Args&&... args) {
    return make_unique<command>(
        logger::configure().create<const logger&>(), forward<Args>(args)...);
  }
}

LEMONBUDDY_NS_END

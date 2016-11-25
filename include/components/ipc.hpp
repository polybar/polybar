#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/concurrency.hpp"
#include "utils/functional.hpp"

POLYBAR_NS

/**
 * Message types
 */
struct ipc_command {
  static constexpr const char* prefix{"cmd:"};
  string payload;
};
struct ipc_hook {
  static constexpr const char* prefix{"hook:"};
  string payload;
};
struct ipc_action {
  static constexpr const char* prefix{"action:"};
  string payload;
};

/**
 * Component used for inter-process communication.
 *
 * A unique messaging channel will be setup for each
 * running process which will allow messages and
 * events to be sent to the process externally.
 */
class ipc {
 public:
  explicit ipc(const logger& logger) : m_log(logger) {}
  ~ipc();

  void attach_callback(callback<const ipc_command&>&& cb);
  void attach_callback(callback<const ipc_hook&>&& cb);
  void attach_callback(callback<const ipc_action&>&& cb);
  void receive_messages();

 protected:
  void parse(const string& payload) const;
  void delegate(const ipc_command& message) const;
  void delegate(const ipc_hook& message) const;
  void delegate(const ipc_action& message) const;

 private:
  const logger& m_log;

  vector<callback<const ipc_command&>> m_command_callbacks;
  vector<callback<const ipc_hook&>> m_hook_callbacks;
  vector<callback<const ipc_action&>> m_action_callbacks;

  stateflag m_running{false};

  string m_fifo;
  int m_fd;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<ipc>>
  di::injector<T> configure_ipc() {
    return di::make_injector(configure_logger());
  }
}

POLYBAR_NS_END

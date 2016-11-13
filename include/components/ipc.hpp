#pragma once

#include "common.hpp"
#include "components/logger.hpp"

LEMONBUDDY_NS

/**
 * Component used for inter-process communication.
 *
 * A unique messaging channel will be setup for each
 * running process which will allow messages and
 * events to be sent to the process externally.
 */
class ipc {
 public:
  struct message_internal {
    static constexpr auto prefix{"app:"};
  };
  struct message_command {
    static constexpr auto prefix{"cmd:"};
  };
  struct message_custom {
    static constexpr auto prefix{"custom:"};
  };

  explicit ipc(const logger& logger) : m_log(logger) {}
  ~ipc();

  void receive_messages();

 protected:
  void parse(string payload);

 private:
  const logger& m_log;

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

LEMONBUDDY_NS_END

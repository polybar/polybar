#pragma once

#include "common.hpp"
#include "components/logger.hpp"
#include "events/signal_emitter.hpp"
#include "utils/concurrency.hpp"
#include "utils/functional.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

/**
 * Message types
 */
struct ipc_command {
  static constexpr const char* prefix{"cmd:"};
  char payload[EVENT_SIZE]{'\0'};
};
struct ipc_hook {
  static constexpr const char* prefix{"hook:"};
  char payload[EVENT_SIZE]{'\0'};
};
struct ipc_action {
  static constexpr const char* prefix{"action:"};
  char payload[EVENT_SIZE]{'\0'};
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
  explicit ipc(signal_emitter& emitter, const logger& logger) : m_sig(emitter), m_log(logger) {}
  ~ipc();

  void receive_messages();

 protected:
  void parse(const string& payload) const;

 private:
  signal_emitter& m_sig;
  const logger& m_log;
  string m_fifo;
  int m_fd;
  stateflag m_running{false};
};

namespace {
  /**
   * Configure injection module
   */
  inline unique_ptr<ipc> make_ipc() {
    return factory_util::unique<ipc>(make_signal_emitter(), make_logger());
  }
}

POLYBAR_NS_END

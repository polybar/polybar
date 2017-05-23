#pragma once

#include "common.hpp"
#include "settings.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

class file_descriptor;
class logger;
class signal_emitter;

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
  using make_type = unique_ptr<ipc>;
  static make_type make();

  explicit ipc(signal_emitter& emitter, const logger& logger);
  ~ipc();

  void receive_message();
  int get_file_descriptor() const;

 private:
  signal_emitter& m_sig;
  const logger& m_log;

  string m_path{};
  unique_ptr<file_descriptor> m_fd;
};

POLYBAR_NS_END

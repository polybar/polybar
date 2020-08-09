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
static constexpr const char* ipc_command_prefix{"cmd:"};
static constexpr const char* ipc_hook_prefix{"hook:"};
static constexpr const char* ipc_action_prefix{"action:"};

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

#pragma once

#include <uv.h>

#include "common.hpp"
#include "settings.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

class signal_emitter;
class logger;

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

  void receive_data(string buf);
  void receive_eof();
  int get_file_descriptor() const;

 private:
  signal_emitter& m_sig;
  const logger& m_log;

  string m_path{};
  int m_fd;

  /**
   * Buffer for the currently received IPC message.
   */
  string m_buffer{};
};

POLYBAR_NS_END

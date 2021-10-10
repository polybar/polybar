#pragma once

#include <uv.h>

#include "common.hpp"
#include "components/eventloop.hpp"
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
  static make_type make(eventloop& loop);

  explicit ipc(signal_emitter& emitter, const logger& logger, eventloop& loop);
  ~ipc();

  string get_path() const;

  void receive_data(string buf);
  void receive_eof();

 private:
  signal_emitter& m_sig;
  const logger& m_log;
  eventloop& m_loop;

  string m_path{};

  /**
   * Buffer for the currently received IPC message.
   */
  string m_buffer{};
};

POLYBAR_NS_END

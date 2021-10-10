#include "components/ipc.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "components/eventloop.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Message types
 */
static constexpr const char* ipc_command_prefix{"cmd:"};
static constexpr const char* ipc_hook_prefix{"hook:"};
static constexpr const char* ipc_action_prefix{"action:"};

/**
 * Create instance
 */
ipc::make_type ipc::make(eventloop& loop) {
  return std::make_unique<ipc>(signal_emitter::make(), logger::make(), loop);
}

/**
 * Construct ipc handler
 */
ipc::ipc(signal_emitter& emitter, const logger& logger, eventloop& loop) : m_sig(emitter), m_log(logger), m_loop(loop) {
  m_pipe_path = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

  if (file_util::exists(m_pipe_path) && unlink(m_pipe_path.c_str()) == -1) {
    throw system_error("Failed to remove ipc channel");
  }
  if (mkfifo(m_pipe_path.c_str(), 0666) == -1) {
    throw system_error("Failed to create ipc channel");
  }
  m_log.info("Created ipc channel at: %s", m_pipe_path);

  m_loop.named_pipe_handle(
      m_pipe_path, [this](const string payload) { receive_data(payload); }, [this]() { receive_eof(); },
      [this](int err) { m_log.err("libuv error while listening to IPC channel: %s", uv_strerror(err)); });
}

/**
 * Deconstruct ipc handler
 */
ipc::~ipc() {
  m_log.trace("ipc: Removing named pipe at: %s", m_pipe_path);
  if (unlink(m_pipe_path.c_str()) == -1) {
    m_log.err("Failed to delete ipc named pipe: %s", strerror(errno));
  }
}

void ipc::trigger_ipc(const string& msg) {
  if (msg.find(ipc_command_prefix) == 0) {
    m_sig.emit(signals::ipc::command{msg.substr(strlen(ipc_command_prefix))});
  } else if (msg.find(ipc_hook_prefix) == 0) {
    m_sig.emit(signals::ipc::hook{msg.substr(strlen(ipc_hook_prefix))});
  } else if (msg.find(ipc_action_prefix) == 0) {
    m_sig.emit(signals::ipc::action{msg.substr(strlen(ipc_action_prefix))});
  } else {
    m_log.warn("Received unknown ipc message: (payload=%s)", msg);
  }
}

/**
 * Receive parts of an IPC message
 */
void ipc::receive_data(string buf) {
  m_pipe_buffer += buf;

  m_log.warn("Using the named pipe at '%s' for ipc is deprecated, always use 'polybar-msg'", m_pipe_path);
}

/**
 * Called once the end of the message arrives.
 */
void ipc::receive_eof() {
  if (m_pipe_buffer.empty()) {
    return;
  }

  trigger_ipc(string_util::trim(std::move(m_pipe_buffer), '\n'));
  m_pipe_buffer.clear();
}

POLYBAR_NS_END

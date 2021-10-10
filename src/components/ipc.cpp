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
  m_path = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

  if (file_util::exists(m_path) && unlink(m_path.c_str()) == -1) {
    throw system_error("Failed to remove ipc channel");
  }
  if (mkfifo(m_path.c_str(), 0666) == -1) {
    throw system_error("Failed to create ipc channel");
  }
  m_log.info("Created ipc channel at: %s", m_path);

  m_loop.named_pipe_handle(
      get_path(), [this](const string payload) { receive_data(payload); }, [this]() { receive_eof(); },
      [this](int err) { m_log.err("libuv error while listening to IPC channel: %s", uv_strerror(err)); });
}

/**
 * Deconstruct ipc handler
 */
ipc::~ipc() {
  m_log.trace("ipc: Removing file handle at: %s", m_path);
  unlink(m_path.c_str());
}

string ipc::get_path() const {
  return m_path;
}

/**
 * Receive parts of an IPC message
 */
void ipc::receive_data(string buf) {
  m_buffer += buf;
}

/**
 * Called once the end of the message arrives.
 */
void ipc::receive_eof() {
  if (m_buffer.empty()) {
    return;
  }

  string payload{string_util::trim(std::move(m_buffer), '\n')};

  m_buffer = std::string();

  if (payload.find(ipc_command_prefix) == 0) {
    m_sig.emit(signals::ipc::command{payload.substr(strlen(ipc_command_prefix))});
  } else if (payload.find(ipc_hook_prefix) == 0) {
    m_sig.emit(signals::ipc::hook{payload.substr(strlen(ipc_hook_prefix))});
  } else if (payload.find(ipc_action_prefix) == 0) {
    m_sig.emit(signals::ipc::action{payload.substr(strlen(ipc_action_prefix))});
  } else {
    m_log.warn("Received unknown ipc message: (payload=%s)", payload);
  }
}

POLYBAR_NS_END

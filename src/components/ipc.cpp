#include <fcntl.h>
#include <sys/stat.h>

#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Create instance
 */
ipc::make_type ipc::make() {
  return factory_util::unique<ipc>(signal_emitter::make(), logger::make());
}

/**
 * Construct ipc handler
 */
ipc::ipc(signal_emitter& emitter, const logger& logger) : m_sig(emitter), m_log(logger) {
  m_path = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

  if (file_util::exists(m_path) && unlink(m_path.c_str()) == -1) {
    throw system_error("Failed to remove ipc channel");
  }
  if (mkfifo(m_path.c_str(), 0666) == -1) {
    throw system_error("Failed to create ipc channel");
  }

  m_log.info("Created ipc channel at: %s", m_path);
  m_fd = file_util::make_file_descriptor(m_path, O_RDONLY | O_NONBLOCK);
}

/**
 * Deconstruct ipc handler
 */
ipc::~ipc() {
  m_fd.reset();

  if (!m_path.empty()) {
    m_log.trace("ipc: Removing file handle");
    unlink(m_path.c_str());
  }
}

/**
 * Receive available ipc messages and delegate valid events
 */
void ipc::receive_message() {
  m_log.info("Receiving ipc message");

  char buffer[BUFSIZ]{'\0'};
  ssize_t bytes_read{0};

  if ((bytes_read = read(*m_fd, &buffer, BUFSIZ)) == -1) {
    m_log.err("Failed to read from ipc channel (err: %s)", strerror(errno));
  } else if (bytes_read > 0) {
    string payload{string_util::trim(string{buffer}, '\n')};

    if (payload.find(ipc_command_prefix) == 0) {
      m_sig.emit(signals::ipc::command{payload.substr(strlen(ipc_command_prefix))});
    } else if (payload.find(ipc_hook_prefix) == 0) {
      m_sig.emit(signals::ipc::hook{payload.substr(strlen(ipc_hook_prefix))});
    } else if (payload.find(ipc_action_prefix) == 0) {
      m_sig.emit(signals::ipc::action{payload.substr(strlen(ipc_action_prefix))});
    } else if (!payload.empty()) {
      m_log.warn("Received unknown ipc message: (payload=%s)", payload);
    }
  }

  m_fd = file_util::make_file_descriptor(m_path, O_RDONLY | O_NONBLOCK);
}

/**
 * Get the file descriptor to the ipc channel
 */
int ipc::get_file_descriptor() const {
  return *m_fd;
}

POLYBAR_NS_END

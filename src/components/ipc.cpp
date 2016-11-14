#include <fcntl.h>
#include <sys/stat.h>

#include "components/ipc.hpp"
#include "config.hpp"
#include "utils/file.hpp"
#include "utils/io.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

/**
 * Interrupt the blocked listener and
 * remove the file handler
 */
ipc::~ipc() {
  m_running = false;

  if (!m_fifo.empty()) {
    m_log.info("Interrupting ipc message receiver");

    auto f{make_unique<file_util::file_ptr>(m_fifo)};
    char p[1]{'q'};

    fwrite(p, sizeof(char), sizeof(p), (*f)());
    unlink(m_fifo.c_str());
  }
}

/**
 * Register listener callback for ipc_command messages
 */
void ipc::attach_callback(callback<const ipc_command&>&& cb) {
  m_command_callbacks.emplace_back(cb);
}

/**
 * Register listener callback for ipc_hook messages
 */
void ipc::attach_callback(callback<const ipc_hook&>&& cb) {
  m_hook_callbacks.emplace_back(cb);
}

/**
 * Start listening for event messages
 */
void ipc::receive_messages() {
  m_running = true;
  m_fifo = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

  if (mkfifo(m_fifo.c_str(), 0666) == -1) {
    m_log.err("Failed to create messaging channel");
  }

  m_log.info("Listening for ipc messages on: %s", m_fifo);

  while ((m_fd = open(m_fifo.c_str(), O_RDONLY)) != -1 && m_running) {
    parse(io_util::readline(m_fd));
    close(m_fd);
  }
}

/**
 * Process received message and delegate
 * valid events to the target modules
 */
void ipc::parse(const string& payload) const {
  if (payload.empty()) {
    return;
  } else if (payload.find(ipc_command::prefix) == 0) {
    delegate(ipc_command{payload});
  } else if (payload.find(ipc_hook::prefix) == 0) {
    delegate(ipc_hook{payload});
  } else {
    m_log.warn("Received unknown ipc message: (payload=%s)", payload);
  }
}

/**
 * Send ipc message to attached listeners
 */
void ipc::delegate(const ipc_command& message) const {
  if (!m_command_callbacks.empty())
    for (auto&& callback : m_command_callbacks) callback(message);
  else
    m_log.warn("Unhandled message (payload=%s)", message.payload);
}

/**
 * Send ipc message to attached listeners
 */
void ipc::delegate(const ipc_hook& message) const {
  if (!m_hook_callbacks.empty())
    for (auto&& callback : m_hook_callbacks) callback(message);
  else
    m_log.warn("Unhandled message (payload=%s)", message.payload);
}

LEMONBUDDY_NS_END

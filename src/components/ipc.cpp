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
void ipc::parse(string payload) {
  if (payload.empty()) {
    return;
  } else if (payload.find(message_internal::prefix) == 0) {
    m_log.info("Received internal message: (payload=%s)", payload);
  } else if (payload.find(message_command::prefix) == 0) {
    m_log.info("Received command message: (payload=%s)", payload);
  } else if (payload.find(message_custom::prefix) == 0) {
    m_log.info("Received custom message: (payload=%s)", payload);
  } else {
    m_log.info("Received unknown message: (payload=%s)", payload);
  }
}

LEMONBUDDY_NS_END

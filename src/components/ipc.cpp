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

namespace ipc {

  /**
   * Message types
   */
  static constexpr const char* ipc_command_prefix{"cmd:"};
  static constexpr const char* ipc_hook_prefix{"hook:"};
  static constexpr const char* ipc_action_prefix{"action:"};

  /**
   * Create instance
   */
  ipc::make_type ipc::make(eventloop::eventloop& loop) {
    return std::make_unique<ipc>(signal_emitter::make(), logger::make(), loop);
  }

  /**
   * Construct ipc handler
   */
  ipc::ipc(signal_emitter& emitter, const logger& logger, eventloop::eventloop& loop)
      : m_sig(emitter), m_log(logger), m_loop(loop) {
    m_pipe_path = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

    if (file_util::exists(m_pipe_path) && unlink(m_pipe_path.c_str()) == -1) {
      throw system_error("Failed to remove ipc channel");
    }
    if (mkfifo(m_pipe_path.c_str(), 0666) == -1) {
      throw system_error("Failed to create ipc channel");
    }
    m_log.info("Created ipc channel at: %s", m_pipe_path);

    m_loop.named_pipe_handle(
        m_pipe_path, [this](const char* buf, size_t size) { receive_data(string(buf, size)); },
        [this]() { receive_eof(); },
        [this](int err) { m_log.err("libuv error while listening to IPC channel: %s", uv_strerror(err)); });

    // TODO socket path
    socket = m_loop.socket_handle(
        "test.sock", 4, [this]() { on_connection(); },
        [this](int err) { m_log.err("libuv error while listening to IPC socket: %s", uv_strerror(err)); });
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

  void ipc::on_connection() {
    // TODO
    m_log.notice("New connection");
    auto ipc_client = make_shared<client>(logger::make());
    auto client_pipe = m_loop.pipe_handle();
    clients.emplace(client_pipe, ipc_client);

    socket->accept(*client_pipe);

    /*
     * We need a weak_ptr here because otherwise, client_pipe would hold a
     * lambda that captures itself which could never get deleted.
     */
    auto weak_pipe = std::weak_ptr<eventloop::PipeHandle>(client_pipe);

    client_pipe->start(
        [this, ipc_client, weak_pipe](const char* data, size_t size) {
          bool success = ipc_client->on_read(data, size);
          if (!success) {
            remove_client(weak_pipe);
          }
        },
        [this, weak_pipe]() { remove_client(weak_pipe); },
        [this, weak_pipe](int err) {
          m_log.err("ipc: libuv error while listening to IPC socket: %s", uv_strerror(err));
          remove_client(weak_pipe);
        });

    // TODO
    m_log.notice("%d open clients", clients.size());
  }

  void ipc::remove_client(std::weak_ptr<eventloop::PipeHandle> pipe) {
    auto shared = pipe.lock();
    shared->close();
    clients.erase(shared);
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
}  // namespace ipc

POLYBAR_NS_END

#include "components/ipc.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cassert>

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
      : m_sig(emitter), m_log(logger), m_loop(loop), socket(loop.handle<eventloop::PipeHandle>()) {
    m_pipe_path = string_util::replace(PATH_MESSAGING_FIFO, "%pid%", to_string(getpid()));

    if (file_util::exists(m_pipe_path) && unlink(m_pipe_path.c_str()) == -1) {
      throw system_error("Failed to remove ipc channel");
    }
    if (mkfifo(m_pipe_path.c_str(), 0666) == -1) {
      throw system_error("Failed to create ipc channel");
    }
    m_log.info("Created ipc channel at: %s", m_pipe_path);

    if ((fd = open(m_pipe_path.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
      throw system_error("Failed to open pipe '" + m_pipe_path + "'");
    }

    ipc_pipe = make_unique<fifo>(m_loop, *this, fd);

    // TODO socket path
    socket.bind("test.sock");
    socket.listen(
        4, [this]() { on_connection(); },
        [this](const auto& e) {
          m_log.err("libuv error while listening to IPC socket: %s", uv_strerror(e.status));
          socket.close();
        });
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
    m_log.info("Received ipc message: '%s'", msg);
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
    auto connection = make_shared<ipc::connection>(m_loop, [this](uint8_t version, const vector<uint8_t>& msg) {
      // Right now, the ipc_client only accepts a single version
      assert(version == VERSION);
      string str;
      str.insert(str.end(), msg.begin(), msg.end());
      trigger_ipc(str);
      // TODO writeback success/error message
    });

    connections.emplace(connection);
    m_log.info("ipc: New connection (%d clients)", connections.size());

    socket.accept(connection->client_pipe);

    connection->client_pipe.read_start(
        [this, connection](const auto& e) {
          bool success = connection->decoder.on_read((const uint8_t*)e.data, e.len);
          if (!success) {
            remove_client(connection);
          }
        },
        [this, connection]() { remove_client(connection); },
        [this, connection](const auto& e) {
          m_log.err("ipc: libuv error while listening to IPC socket: %s", uv_strerror(e.status));
          remove_client(connection);
        });
  }

  void ipc::remove_client(shared_ptr<connection> conn) {
    conn->client_pipe.close();
    connections.erase(conn);
  }

  ipc::connection::connection(eventloop::eventloop& loop, client::cb msg_callback)
      : client_pipe(loop.handle<eventloop::PipeHandle>()), decoder(logger::make(), msg_callback) {}

  ipc::fifo::fifo(eventloop::eventloop& loop, ipc& ipc, int fd) : pipe_handle(loop.handle<eventloop::PipeHandle>()) {
    pipe_handle.open(fd);
    pipe_handle.read_start([&ipc](const auto& e) mutable { ipc.receive_data(string(e.data, e.len)); },
        [&ipc]() mutable { ipc.receive_eof(); },
        [this, &ipc](const auto& e) mutable {
          ipc.m_log.err("libuv error while listening to IPC channel: %s", uv_strerror(e.status));
          pipe_handle.close();
        });
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
    ipc_pipe = make_unique<fifo>(m_loop, *this, fd);

    if (m_pipe_buffer.empty()) {
      return;
    }

    trigger_ipc(string_util::trim(std::move(m_pipe_buffer), '\n'));
    m_pipe_buffer.clear();
  }
}  // namespace ipc

POLYBAR_NS_END

#include "ipc/ipc.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cassert>

#include "components/eventloop.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "ipc/util.hpp"
#include "utils/env.hpp"
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
    if (mkfifo(m_pipe_path.c_str(), 0600) == -1) {
      throw system_error("Failed to create ipc channel");
    }
    m_log.info("Created ipc channel at: %s", m_pipe_path);

    ipc_pipe = make_unique<fifo>(m_loop, *this, m_pipe_path);

    ensure_runtime_path();
    string sock_path = get_socket_path(getpid());

    m_log.info("Opening ipc socket at '%s'", sock_path);
    m_log.notice("Listening for IPC messages (PID: %d)", getpid());
    socket.bind(sock_path);
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

    socket.close();
    // TODO delete runtime directory
  }

  string ipc::get_socket_path(int pid) {
    return ensure_runtime_path() + "/ipc." + to_string(pid) + ".sock";
  }

  void ipc::trigger_ipc(v0::ipc_type type, const string& msg) {
    switch (type) {
      case v0::ipc_type::CMD:
        m_log.info("Received ipc command: '%s'", msg);
        // TODO should return an error
        m_sig.emit(signals::ipc::command{msg});
        break;
      case v0::ipc_type::ACTION:
        m_log.info("Received ipc action: '%s'", msg);
        m_sig.emit(signals::ipc::action{msg});
        break;
    }
  }

  void ipc::trigger_legacy_ipc(const string& msg) {
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
    auto connection = make_unique<ipc::connection>(
        m_loop, [this](ipc::connection& c, uint8_t, v0::ipc_type type, const vector<uint8_t>& msg) {
          string str;
          str.insert(str.end(), msg.begin(), msg.end());
          // TODO should return an error code
          trigger_ipc(type, str);
          // TODO use message format for response
          c.client_pipe.write((const uint8_t*)"SUCCESS", 7, [this, &c]() { remove_client(c); },
              [this, &c](const auto& e) {
                m_log.err("ipc: libuv error while writing to IPC socket: %s", uv_strerror(e.status));
                remove_client(c);
              });
          c.client_pipe.close();
        });

    auto& c = *connection;
    socket.accept(c.client_pipe);

    c.client_pipe.read_start(
        [this, &c](const auto& e) {
          try {
            c.dec.on_read((const uint8_t*)e.data, e.len);
          } catch (const decoder::error& e) {
            m_log.err("ipc: %s", e.what());
            // TODO write back some error message
            remove_client(c);
          }
        },
        [this, &c]() { remove_client(c); },
        [this, &c](const auto& e) {
          m_log.err("ipc: libuv error while listening to IPC socket: %s", uv_strerror(e.status));
          remove_client(c);
        });

    connections.emplace(std::move(connection));
    m_log.info("ipc: New connection (%d clients)", connections.size());
  }

  void ipc::remove_client(connection& conn) {
    conn.client_pipe.close();
    connections.erase(connections.find(conn));
  }

  ipc::connection::connection(eventloop::eventloop& loop, cb msg_callback)
      : client_pipe(loop.handle<eventloop::PipeHandle>())
      , dec(logger::make(), [this, msg_callback](uint8_t version, auto type, const auto& msg) {
        msg_callback(*this, version, type, msg);
      }) {}

  ipc::fifo::fifo(eventloop::eventloop& loop, ipc& ipc, const string& path)
      : pipe_handle(loop.handle<eventloop::PipeHandle>()) {
    int fd;
    if ((fd = open(path.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
      throw system_error("Failed to open pipe '" + path + "'");
    }

    pipe_handle.open(fd);
    pipe_handle.read_start([&ipc](const auto& e) mutable { ipc.receive_data(string(e.data, e.len)); },
        [&ipc]() { ipc.receive_eof(); },
        [this, &ipc](const auto& e) mutable {
          ipc.m_log.err("libuv error while listening to IPC channel: %s", uv_strerror(e.status));
          pipe_handle.close();
        });
  }

  ipc::fifo::~fifo() {
    pipe_handle.close();
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
    ipc_pipe = make_unique<fifo>(m_loop, *this, m_pipe_path);

    if (m_pipe_buffer.empty()) {
      return;
    }

    trigger_legacy_ipc(string_util::trim(std::move(m_pipe_buffer), '\n'));
    m_pipe_buffer.clear();
  }
}  // namespace ipc

POLYBAR_NS_END

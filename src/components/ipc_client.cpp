#include "components/ipc_client.hpp"

#include <cassert>
#include <cstring>

POLYBAR_NS

namespace ipc {

  client::client(const logger& logger, eventloop::SocketHandle& socket, eventloop::PipeHandle_t pipe)
      : m_log(logger), server(socket), client_pipe(pipe) {}

  void client::start() {
    server.accept(*client_pipe);

    client_pipe->start([this](const char* data, size_t size) { on_read(data, size); }, [this]() { on_eof(); },
        [this](int err) {
          state = client_state::ERR;
          m_log.err("libuv error while listening to IPC socket: %s", uv_strerror(err));
          stop();
        });
  }

  void client::on_read(const char* data, size_t size) {
    if (state != client_state::WAIT && state != client_state::READ) {
      return;
    }

    m_log.notice("Received %d bytes", size);

    size_t buf_pos = 0;
    size_t remain = size;

    while (remain > 0) {
      if (state == client_state::WAIT) {
        ssize_t num_read = process_header_data(data + buf_pos, remain);

        if (num_read < 0) {
          stop();
          return;
        }

        assert(num_read > 0);
        assert(remain >= (size_t)num_read);

        buf_pos += num_read;
        remain -= num_read;
      } else {
        assert(to_read_header == 0);
        ssize_t num_read = process_msg_data(data + buf_pos, remain);

        if (num_read < 0) {
          stop();
          return;
        }

        assert(num_read > 0);
        assert(remain >= (size_t)num_read);

        buf_pos += num_read;
        remain -= num_read;
      }
    }
  }

  /**
   * If we are waiting for header data, read as many bytes as possible from the given buffer.
   *
   * \return Number of bytes processed. -1 for errors
   */
  ssize_t client::process_header_data(const char* data, size_t size) {
    assert(state == client_state::WAIT);
    assert(to_read_header > 0);

    size_t num_read = std::min(size, to_read_header);

    memcpy(&header.d[HEADER_SIZE - to_read_header], data, num_read);
    to_read_header -= num_read;

    if (to_read_header == 0) {
      if (memcmp(header.s.magic, MAGIC, MAGIC_SIZE) != 0) {
        m_log.err("ipc: Invalid magic header, expected '%s', got '%.*s'", MAGIC, MAGIC_SIZE, header.s.magic);
        return -1;
      }

      uint8_t version = header.s.version;
      uint32_t msg_size = header.s.size;

      if (version != VERSION) {
        m_log.err("ipc: Unsupported message format version %d", version);
        return -1;
      }

      // TODO
      m_log.notice("Message size: %d", msg_size);

      assert(buf.empty());
      state = client_state::READ;
      to_read_buf = msg_size;
    }

    return num_read;
  }

  /**
   * If we are waiting for message data, read as many bytes as possible from the given buffer.
   *
   * \return Number of bytes processed. -1 for errors
   */
  ssize_t client::process_msg_data(const char* data, size_t size) {
    assert(state == client_state::READ);

    size_t num_read = std::min(size, to_read_buf);

    buf.reserve(buf.size() + num_read);
    for (size_t i = 0; i < num_read; i++) {
      buf.push_back((uint8_t)data[i]);
    }
    to_read_buf -= num_read;

    // TODO dispatch message
    string msg;
    msg.reserve(buf.size());

    for (uint8_t e : buf) {
      msg.push_back((char)e);
    }

    m_log.notice("Received ipc message: '%s'", msg);

    if (to_read_buf == 0) {
      state = client_state::WAIT;
      to_read_header = HEADER_SIZE;
      buf.clear();
    }

    return num_read;
  }

  void client::on_eof() {
    state = client_state::DONE;
    m_log.notice("Received EOF");
    stop();
  }

  void client::stop() {
    client_pipe->close();
    // TODO remove pipe
  }
}  // namespace ipc
POLYBAR_NS_END

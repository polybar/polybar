#include "ipc/client.hpp"

#include <cassert>
#include <cstring>

POLYBAR_NS

namespace ipc {
  client::client(const logger& logger, cb callback) : callback(callback), m_log(logger) {}

  void client::on_read(const uint8_t* data, size_t size) {
    if (state == client_state::CLOSED) {
      throw error("Decoder is closed");
    }

    try {
      process_data(data, size);
    } catch (const error& e) {
      close();
      throw;
    }
  }

  void client::process_data(const uint8_t* data, size_t size) {
    m_log.trace("ipc: Received %zd bytes", size);

    size_t buf_pos = 0;
    size_t remain = size;

    while (remain > 0) {
      if (state == client_state::WAIT) {
        ssize_t num_read = process_header_data(data + buf_pos, remain);
        assert(num_read > 0);
        assert(remain >= (size_t)num_read);

        buf_pos += num_read;
        remain -= num_read;
      } else {
        assert(to_read_header == 0);
        ssize_t num_read = process_msg_data(data + buf_pos, remain);
        assert(num_read > 0);
        assert(remain >= (size_t)num_read);

        buf_pos += num_read;
        remain -= num_read;
      }
    }
  }

  void client::close() noexcept {
    state = client_state::CLOSED;
  }

  /**
   * If we are waiting for header data, read as many bytes as possible from the given buffer.
   *
   * \return Number of bytes processed. -1 for errors
   */
  ssize_t client::process_header_data(const uint8_t* data, size_t size) {
    assert(state == client_state::WAIT);
    assert(to_read_header > 0);

    size_t num_read = std::min(size, to_read_header);

    std::copy(data, data + num_read, header.d + HEADER_SIZE - to_read_header);
    to_read_header -= num_read;

    if (to_read_header == 0) {
      uint8_t version = header.s.version;
      uint32_t msg_size = header.s.size;
      type_t type = header.s.type;

      m_log.trace(
          "Received full ipc header (magic=%.*s version=%d size=%zd)", MAGIC.size(), header.s.magic, version, msg_size);

      if (memcmp(header.s.magic, MAGIC.data(), MAGIC.size()) != 0) {
        throw error("Invalid magic header, expected '" + MAGIC_STR + "', got '" +
                    string(reinterpret_cast<const char*>(header.s.magic), MAGIC.size()));
      }
      if (version != VERSION) {
        throw error("Unsupported message format version " + to_string(version));
      }

      if (type != to_integral(v0::ipc_type::ACTION) && type != to_integral(v0::ipc_type::CMD)) {
        throw error("Unsupported message type " + to_string(type));
      }

      ipc_type = static_cast<v0::ipc_type>(type);

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
  ssize_t client::process_msg_data(const uint8_t* data, size_t size) {
    assert(state == client_state::READ);

    size_t num_read = std::min(size, to_read_buf);

    buf.reserve(buf.size() + num_read);
    for (size_t i = 0; i < num_read; i++) {
      buf.push_back(data[i]);
    }
    to_read_buf -= num_read;

    if (to_read_buf == 0) {
      callback(header.s.version, ipc_type, buf);

      state = client_state::WAIT;
      to_read_header = HEADER_SIZE;
      buf.clear();
    }

    return num_read;
  }
}  // namespace ipc
POLYBAR_NS_END

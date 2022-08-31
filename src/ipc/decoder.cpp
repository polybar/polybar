#include "ipc/decoder.hpp"

#include <cassert>
#include <cstring>

POLYBAR_NS

namespace ipc {
  decoder::decoder(const logger& logger, cb callback) : callback(callback), m_log(logger) {}

  void decoder::on_read(const uint8_t* data, size_t size) {
    if (state == state::CLOSED) {
      throw error("Decoder is closed");
    }

    try {
      process_data(data, size);
    } catch (const error& e) {
      close();
      throw;
    }
  }

  void decoder::process_data(const uint8_t* data, size_t size) {
    m_log.trace("ipc: Received %zd bytes", size);

    size_t buf_pos = 0;
    size_t remain = size;

    while (remain > 0) {
      if (state == state::HEADER) {
        ssize_t num_read = process_header_data(data + buf_pos, remain);
        assert(num_read > 0);
        assert(remain >= (size_t)num_read);

        buf_pos += num_read;
        remain -= num_read;

        /*
         * If an empty message arrives, we need to explicitly trigger this because there is no further data that would
         * call process_msg_data.
         */
        if (remain == 0 && state == state::PAYLOAD && to_read_buf == 0) {
          ssize_t num_read_data = process_msg_data(data + buf_pos, remain);
          assert(num_read_data == 0);
          (void)num_read_data;
        }

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

  void decoder::close() noexcept {
    state = state::CLOSED;
  }

  bool decoder::closed() const {
    return state == state::CLOSED;
  }

  /**
   * If we are waiting for header data, read as many bytes as possible from the given buffer.
   *
   * @return Number of bytes processed.
   * @throws decoder::error on message errors
   */
  ssize_t decoder::process_header_data(const uint8_t* data, size_t size) {
    assert(state == state::HEADER);
    assert(to_read_header > 0);

    size_t num_read = std::min(size, to_read_header);

    std::copy(data, data + num_read, header.d + HEADER_SIZE - to_read_header);
    to_read_header -= num_read;

    if (to_read_header == 0) {
      uint8_t version = header.s.version;
      uint32_t msg_size = header.s.size;

      m_log.trace(
          "Received full ipc header (magic=%.*s version=%d size=%zd)", MAGIC.size(), header.s.magic, version, msg_size);

      if (memcmp(header.s.magic, MAGIC.data(), MAGIC.size()) != 0) {
        throw error("Invalid magic header, expected '" + MAGIC_STR + "', got '" +
                    string(reinterpret_cast<const char*>(header.s.magic), MAGIC.size()) + "'");
      }
      if (version != VERSION) {
        throw error("Unsupported message format version " + to_string(version));
      }

      assert(buf.empty());
      state = state::PAYLOAD;
      to_read_buf = msg_size;
    }

    return num_read;
  }

  /**
   * If we are waiting for message data, read as many bytes as possible from the given buffer.
   *
   * @return Number of bytes processed.
   * @throws decoder::error on message errors
   */
  ssize_t decoder::process_msg_data(const uint8_t* data, size_t size) {
    assert(state == state::PAYLOAD);

    size_t num_read = std::min(size, to_read_buf);

    buf.reserve(buf.size() + num_read);
    for (size_t i = 0; i < num_read; i++) {
      buf.push_back(data[i]);
    }
    to_read_buf -= num_read;

    if (to_read_buf == 0) {
      callback(header.s.version, header.s.type, buf);

      state = state::HEADER;
      to_read_header = HEADER_SIZE;
      buf.clear();
    }

    return num_read;
  }
}  // namespace ipc
POLYBAR_NS_END

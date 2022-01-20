#pragma once

#include <queue>

#include "common.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "ipc/msg.hpp"

POLYBAR_NS

namespace ipc {

  /**
   * Decoder for the IPC message format.
   */
  class client {
   public:
    DEFINE_ERROR(error);
    /**
     * Callback is called whenever a full message is received.
     * The message version, message type, and the data is passed.
     */
    using cb = std::function<void(uint8_t, v0::ipc_type, const std::vector<uint8_t>&)>;
    client(const logger&, cb callback);

    /**
     * Call this function whenever new data arrives.
     *
     * Will throw client::error in case of error.
     * If an error is thrown, this instance is closed and this function may not be called again.
     */
    void on_read(const uint8_t* buf, size_t size);
    void close() noexcept;

   protected:
    void process_data(const uint8_t*, size_t size);
    ssize_t process_header_data(const uint8_t*, size_t size);
    ssize_t process_msg_data(const uint8_t*, size_t size);

    ipc::header header;
    size_t to_read_header{ipc::HEADER_SIZE};

    v0::ipc_type ipc_type;

    std::vector<uint8_t> buf;
    size_t to_read_buf{0};

    cb callback;

   private:
    enum class client_state {
      // Waiting for new message (header does not contain full header)
      WAIT,
      // Waiting for message data (header contains valid header)
      READ,
      CLOSED,
    } state{client_state::WAIT};
    const logger& m_log;
  };

}  // namespace ipc

POLYBAR_NS_END

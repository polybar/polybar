#pragma once

#include <queue>

#include "common.hpp"
#include "components/ipc_msg.hpp"
#include "components/logger.hpp"

POLYBAR_NS

namespace ipc {

  class client {
   public:
    client(const logger&);

    bool on_read(const char* buf, size_t size);

   protected:
    ssize_t process_header_data(const char*, size_t size);
    ssize_t process_msg_data(const char*, size_t size);

    ipc::header header;
    size_t to_read_header{ipc::HEADER_SIZE};

    std::vector<uint8_t> buf;
    size_t to_read_buf{0};

   private:
    enum class client_state {
      // Waiting for new message (buf does not contain full header)
      WAIT,
      // Waiting for message data
      READ,
    } state{client_state::WAIT};
    const logger& m_log;
  };

}  // namespace ipc

POLYBAR_NS_END

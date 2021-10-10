#pragma once

#include <queue>

#include "common.hpp"
#include "components/ipc_msg.hpp"
#include "components/logger.hpp"
#include "eventloop.hpp"

POLYBAR_NS

namespace ipc {

  class client {
   public:
    client(const logger&, eventloop::SocketHandle&, eventloop::PipeHandle_t);

    void start();

   protected:
    void on_read(const char* buf, size_t size);

    ssize_t process_header_data(const char*, size_t size);
    ssize_t process_msg_data(const char*, size_t size);

    void on_eof();

    void stop();

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
      // Client is disconnected
      DONE,
      // Client errored
      ERR
    } state{client_state::WAIT};
    const logger& m_log;
    eventloop::SocketHandle& server;
    eventloop::PipeHandle_t client_pipe;
  };

}  // namespace ipc

POLYBAR_NS_END

#pragma once

#include <set>

#include "common.hpp"
#include "components/eventloop.hpp"
#include "ipc/decoder.hpp"
#include "settings.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

class signal_emitter;
class logger;

namespace ipc {
  /**
   * Component used for inter-process communication.
   *
   * A unique messaging channel will be setup for each
   * running process which will allow messages and
   * events to be sent to the process externally.
   */
  class ipc : public non_copyable_mixin, public non_movable_mixin {
   public:
    using make_type = unique_ptr<ipc>;
    static make_type make(eventloop::loop& loop);

    explicit ipc(signal_emitter& emitter, const logger& logger, eventloop::loop& loop);
    ~ipc();

    static string get_socket_path(int pid);

   protected:
    bool trigger_ipc(v0::ipc_type type, const string& msg);
    void trigger_legacy_ipc(const string& msg);

    void on_connection();

   private:
    signal_emitter& m_sig;
    const logger& m_log;
    eventloop::loop& m_loop;

    eventloop::pipe_handle_t m_socket;

    class connection : public non_copyable_mixin, public non_movable_mixin {
     public:
      using cb = std::function<void(connection&, uint8_t, type_t, const std::vector<uint8_t>&)>;
      connection(eventloop::loop& loop, cb msg_callback);
      ~connection();
      eventloop::pipe_handle_t client_pipe;
      decoder dec;
    };

    void remove_client(connection& conn);

    /**
     * Custom transparent comparator so that we can lookup and erase connections from their reference.
     */
    struct connection_cmp {
      using is_transparent = std::true_type;
      bool operator()(const unique_ptr<connection>& a, const unique_ptr<connection>& b) const {
        return a.get() < b.get();
      }

      bool operator()(const connection& a, const unique_ptr<connection>& b) const {
        return &a < b.get();
      }

      bool operator()(const unique_ptr<connection>& a, const connection& b) const {
        return a.get() < &b;
      }
    };

    std::set<unique_ptr<connection>, connection_cmp> connections;

    // Named pipe properties (deprecated)
    struct fifo {
      fifo(eventloop::loop& loop, ipc& ipc, const string& path);
      ~fifo();
      eventloop::pipe_handle_t pipe_handle;
    };

    unique_ptr<fifo> ipc_pipe;

    string m_pipe_path{};
    /**
     * Buffer for the currently received IPC message over the named pipe
     */
    string m_pipe_buffer{};
    void receive_data(const string& buf);
    void receive_eof();
  };
} // namespace ipc

POLYBAR_NS_END

#include "components/eventloop.hpp"

#include <cassert>

#include "errors.hpp"

#if !(UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR >= 3)
#error "Polybar requires libuv 1.x and at least version 1.3"
#endif

POLYBAR_NS

namespace eventloop {

/**
 * Runs any libuv function with an integer error code return value and throws an
 * exception on error.
 */
#define UV(fun, ...)                                                                                        \
  do {                                                                                                      \
    int res = fun(__VA_ARGS__);                                                                             \
    if (res < 0) {                                                                                          \
      throw std::runtime_error(                                                                             \
          __FILE__ ":"s + std::to_string(__LINE__) + ": libuv error for '" #fun "': "s + uv_strerror(res)); \
    }                                                                                                       \
  } while (0);

  /**
   * Closes the given wrapper.
   *
   * We have to have distinct cases for all types because we can't just cast to `UVHandleGeneric` without template
   * arguments.
   */
  void close_handle(uv_handle_t* handle) {
    switch (handle->type) {
      case UV_ASYNC:
        static_cast<AsyncHandle*>(handle->data)->close();
        break;
      case UV_FS_EVENT:
        static_cast<FSEventHandle*>(handle->data)->close();
        break;
      case UV_POLL:
        static_cast<PollHandle*>(handle->data)->close();
        break;
      case UV_TIMER:
        static_cast<TimerHandle*>(handle->data)->close();
        break;
      case UV_SIGNAL:
        static_cast<SignalHandle*>(handle->data)->close();
        break;
      case UV_NAMED_PIPE:
        static_cast<PipeHandle*>(handle->data)->close();
        break;
      default:
        assert(false);
    }
  }

  static void alloc_cb(uv_handle_t*, size_t, uv_buf_t* buf) {
    buf->base = new char[BUFSIZ];
    buf->len = BUFSIZ;
  }

  // SignalHandle {{{
  void SignalHandle::init() {
    UV(uv_signal_init, loop(), get());
  }

  void SignalHandle::start(int signum, cb user_cb) {
    this->callback = user_cb;
    UV(uv_signal_start, get(), event_cb<SignalEvent, &SignalHandle::callback>, signum);
  }
  // }}}

  // PollHandle {{{
  void PollHandle::init(int fd) {
    UV(uv_poll_init, loop(), get(), fd);
  }

  void PollHandle::start(int events, cb user_cb, cb_error err_cb) {
    this->callback = user_cb;
    this->err_cb = err_cb;
    UV(uv_poll_start, get(), events, &poll_callback);
  }

  void PollHandle::poll_callback(uv_poll_t* handle, int status, int events) {
    auto& self = cast(handle);
    if (status < 0) {
      self.close();
      self.err_cb(ErrorEvent{status});
      return;
    }

    self.callback(PollEvent{(uv_poll_event)events});
  }
  // }}}

  // FSEventHandle {{{
  void FSEventHandle::init() {
    UV(uv_fs_event_init, loop(), get());
  }

  void FSEventHandle::start(const string& path, int flags, cb user_cb, cb_error err_cb) {
    this->callback = user_cb;
    this->err_cb = err_cb;
    UV(uv_fs_event_start, get(), fs_event_callback, path.c_str(), flags);
  }

  void FSEventHandle::fs_event_callback(uv_fs_event_t* handle, const char* path, int events, int status) {
    auto& self = cast(handle);
    if (status < 0) {
      self.close();
      self.err_cb(ErrorEvent{status});
      return;
    }

    self.callback(FSEvent{path, (uv_fs_event)events});
  }

  // }}}

  // PipeHandle {{{
  void PipeHandle::init(bool ipc) {
    UV(uv_pipe_init, loop(), get(), ipc);
  }

  void PipeHandle::open(int fd) {
    UV(uv_pipe_open, get(), fd);
  }

  void PipeHandle::read_start(cb_read fun, cb_void eof_cb, cb_error err_cb) {
    this->read_callback = fun;
    this->read_eof_cb = eof_cb;
    this->read_err_cb = err_cb;
    UV(uv_read_start, (uv_stream_t*)get(), alloc_cb, read_cb);
  }

  void PipeHandle::read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
    auto& self = cast((uv_pipe_t*)handle);
    /*
     * Wrap pointer so that it gets automatically freed once the function returns (even with exceptions)
     */
    auto buf_ptr = unique_ptr<char[]>(buf->base);
    if (nread > 0) {
      self.read_callback(ReadEvent{buf->base, (size_t)nread});
    } else if (nread < 0) {
      if (nread != UV_EOF) {
        self.close();
        self.read_err_cb(ErrorEvent{(int)nread});
      } else {
        /*
         * The EOF callback is called in the close callback
         * (or directly here if the handle is already closing).
         *
         * TODO how to handle this for sockets connections?
         */
        if (!uv_is_closing((uv_handle_t*)handle)) {
          uv_close((uv_handle_t*)handle, [](uv_handle_t* handle) { cast((uv_pipe_t*)handle).read_eof_cb(); });
        } else {
          self.read_eof_cb();
        }
      }
    }
  }
  // }}}

  // TimerHandle {{{
  void TimerHandle::init() {
    UV(uv_timer_init, loop(), get());
  }

  void TimerHandle::start(uint64_t timeout, uint64_t repeat, cb user_cb) {
    this->callback = user_cb;
    UV(uv_timer_start, get(), void_event_cb<&TimerHandle::callback>, timeout, repeat);
  }

  void TimerHandle::stop() {
    UV(uv_timer_stop, get());
  }
  // }}}

  // AsyncHandle {{{
  void AsyncHandle::init(cb user_cb) {
    this->callback = user_cb;
    UV(uv_async_init, loop(), get(), void_event_cb<&AsyncHandle::callback>);
  }

  void AsyncHandle::send() {
    UV(uv_async_send, get());
  }
  // }}}

  // SocketHandle {{{
  SocketHandle::SocketHandle(
      uv_loop_t* loop, const string& sock_path, cb_void connection_cb, std::function<void(int)> err_cb)
      : UVHandleGeneric([this](int status) { on_connection(status); })
      , path(sock_path)
      , connection_cb(connection_cb)
      , err_cb(err_cb) {
    UV(uv_pipe_init, loop, handle, 0);
    UV(uv_pipe_bind, handle, sock_path.c_str());
  }

  void SocketHandle::listen(int backlog) {
    UV(uv_listen, (uv_stream_t*)handle, backlog, callback);
  }

  void SocketHandle::on_connection(int status) {
    if (status < 0) {
      err_cb(status);
    } else {
      connection_cb();
    }
  }

  void SocketHandle::accept(PipeHandle& pipe) {
    UV(uv_accept, (uv_stream_t*)handle, (uv_stream_t*)pipe.raw());
  }
  // }}}

  // eventloop {{{
  static void close_walk_cb(uv_handle_t* handle, void*) {
    if (!uv_is_closing(handle)) {
      close_handle(handle);
    }
  }

  /**
   * Completely closes everything in the loop.
   *
   * After this function returns, uv_loop_close can be called.
   */
  static void close_loop(uv_loop_t* loop) {
    uv_walk(loop, close_walk_cb, nullptr);
    UV(uv_run, loop, UV_RUN_DEFAULT);
  }

  eventloop::eventloop() {
    m_loop = std::make_unique<uv_loop_t>();
    UV(uv_loop_init, m_loop.get());
    m_loop->data = this;
  }

  eventloop::~eventloop() {
    if (m_loop) {
      try {
        close_loop(m_loop.get());
        UV(uv_loop_close, m_loop.get());
      } catch (const std::exception& e) {
        logger::make().err("%s", e.what());
      }

      m_loop.reset();
    }
  }

  void eventloop::run() {
    UV(uv_run, m_loop.get(), UV_RUN_DEFAULT);
  }

  void eventloop::stop() {
    uv_stop(m_loop.get());
  }

  uv_loop_t* eventloop::get() const {
    return m_loop.get();
  }

  SocketHandle_t eventloop::socket_handle(
      const string& path, int backlog, cb_void connection_cb, std::function<void(int)> err_cb) {
    m_socket_handles.emplace_back(std::make_shared<SocketHandle>(get(), path, connection_cb, err_cb));
    SocketHandle_t h = m_socket_handles.back();
    h->listen(backlog);
    return h;
  }
  // }}}

}  // namespace eventloop

POLYBAR_NS_END

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
  FSEventHandle::FSEventHandle(uv_loop_t* loop, function<void(const char*, uv_fs_event)> fun, cb_status err_cb)
      : UVHandle([this](const char* path, int events, int status) { fs_event_cb(path, events, status); })
      , func(fun)
      , err_cb(err_cb) {
    UV(uv_fs_event_init, loop, handle);
  }

  void FSEventHandle::start(const string& path) {
    UV(uv_fs_event_start, handle, callback, path.c_str(), 0);
  }

  void FSEventHandle::fs_event_cb(const char* path, int events, int status) {
    if (status < 0) {
      close();
      err_cb(status);
      return;
    }

    func(path, (uv_fs_event)events);
  }
  // }}}

  // PipeHandle {{{
  PipeHandle::PipeHandle(uv_loop_t* loop, cb_read fun, cb_void eof_cb, cb_status err_cb)
      : UVHandleGeneric([this](ssize_t nread, const uv_buf_t* buf) { read_cb(nread, buf); })
      , func(fun)
      , eof_cb(eof_cb)
      , err_cb(err_cb) {
    UV(uv_pipe_init, loop, handle, false);
  }

  void PipeHandle::start(cb_read fun, cb_void eof_cb, cb_status err_cb) {
    this->func = fun;
    this->eof_cb = eof_cb;
    this->err_cb = err_cb;
    UV(uv_read_start, (uv_stream_t*)handle, alloc_cb, callback);
  }

  void PipeHandle::read_cb(ssize_t nread, const uv_buf_t* buf) {
    /*
     * Wrap pointer so that it gets automatically freed once the function returns (even with exceptions)
     */
    auto buf_ptr = unique_ptr<char[]>(buf->base);
    if (nread > 0) {
      func(buf->base, nread);
    } else if (nread < 0) {
      if (nread != UV_EOF) {
        close();
        err_cb(nread);
      } else {
        /*
         * The EOF callback is called in the close callback
         * (or directly here if the handle is already closing).
         */
        if (!uv_is_closing((uv_handle_t*)handle)) {
          uv_close((uv_handle_t*)handle, [](uv_handle_t* handle) { static_cast<PipeHandle*>(handle->data)->eof_cb(); });
        } else {
          eof_cb();
        }
      }
    }
  }
  // }}}

  // NamedPipeHandle {{{
  NamedPipeHandle::NamedPipeHandle(uv_loop_t* loop, const string& path, cb_read fun, cb_void eof_cb, cb_status err_cb)
      : PipeHandle(
            loop, fun,
            [this]() {
              this->eof_cb();
              close_cb();
            },
            err_cb)
      , eof_cb(eof_cb)
      , path(path) {}

  void NamedPipeHandle::start() {
    if ((fd = open(path.c_str(), O_RDONLY | O_NONBLOCK)) == -1) {
      throw system_error("Failed to open pipe '" + path + "'");
    }
    UV(uv_pipe_open, handle, fd);
    UV(uv_read_start, (uv_stream_t*)handle, alloc_cb, callback);
  }

  /*
   * This is a special case.
   *
   * Once we read EOF, we no longer receive events for the fd, so we close the
   * entire handle and restart it with a new fd.
   *
   * We reuse the memory for the underlying uv handle
   */
  void NamedPipeHandle::close_cb(void) {
    UV(uv_pipe_init, this->loop(), this->handle, false);
    this->start();
  }
  // }}}

  // TimerHandle {{{
  TimerHandle::TimerHandle(uv_loop_t* loop, cb_void fun) : UVHandle(fun) {
    UV(uv_timer_init, loop, handle);
  }

  void TimerHandle::start(uint64_t timeout, uint64_t repeat, cb_void new_cb) {
    if (new_cb) {
      this->func = new_cb;
    }

    UV(uv_timer_start, handle, callback, timeout, repeat);
  }

  void TimerHandle::stop() {
    UV(uv_timer_stop, handle);
  }
  // }}}

  // AsyncHandle {{{
  AsyncHandle::AsyncHandle(uv_loop_t* loop, cb_void fun) : UVHandle(fun) {
    UV(uv_async_init, loop, handle, callback);
  }

  void AsyncHandle::send() {
    UV(uv_async_send, handle);
  }
  // }}}

  // SocketHandle {{{
  SocketHandle::SocketHandle(uv_loop_t* loop, const string& sock_path, cb_void connection_cb, cb_status err_cb)
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
    UV(uv_accept, (uv_stream_t*)handle, (uv_stream_t*)pipe.handle);
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

  void eventloop::fs_event_handle(const string& path, function<void(const char*, uv_fs_event)> fun, cb_status err_cb) {
    m_fs_event_handles.emplace_back(std::make_unique<FSEventHandle>(get(), fun, err_cb));
    m_fs_event_handles.back()->start(path);
  }

  void eventloop::named_pipe_handle(const string& path, cb_read fun, cb_void eof_cb, cb_status err_cb) {
    m_named_pipe_handles.emplace_back(std::make_unique<NamedPipeHandle>(get(), path, fun, eof_cb, err_cb));
    m_named_pipe_handles.back()->start();
  }

  TimerHandle_t eventloop::timer_handle(cb_void fun) {
    m_timer_handles.emplace_back(std::make_shared<TimerHandle>(get(), fun));
    return m_timer_handles.back();
  }

  AsyncHandle_t eventloop::async_handle(cb_void fun) {
    m_async_handles.emplace_back(std::make_shared<AsyncHandle>(get(), fun));
    return m_async_handles.back();
  }

  SocketHandle_t eventloop::socket_handle(const string& path, int backlog, cb_void connection_cb, cb_status err_cb) {
    m_socket_handles.emplace_back(std::make_shared<SocketHandle>(get(), path, connection_cb, err_cb));
    SocketHandle_t h = m_socket_handles.back();
    h->listen(backlog);
    return h;
  }

  PipeHandle_t eventloop::pipe_handle() {
    return std::make_shared<PipeHandle>(get());
  }
  // }}}

}  // namespace eventloop

POLYBAR_NS_END

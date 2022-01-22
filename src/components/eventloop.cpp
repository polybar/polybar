#include "components/eventloop.hpp"

#include <cassert>

#include "errors.hpp"

#if !(UV_VERSION_MAJOR == 1 && UV_VERSION_MINOR >= 3)
#error "Polybar requires libuv 1.x and at least version 1.3"
#endif

POLYBAR_NS

namespace eventloop {

  /**
   * Closes the given wrapper.
   *
   * We have to have distinct cases for all types because we can't just cast to `Handle` without template
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

  void PipeHandle::bind(const string& path) {
    UV(uv_pipe_bind, get(), path.c_str());
  }

  void PipeHandle::connect(const string& name, cb_connect user_cb, cb_error err_cb) {
    this->connect_callback = user_cb;
    this->connect_err_cb = err_cb;
    uv_pipe_connect(new uv_connect_t(), get(), name.c_str(), connect_cb);
  }

  void PipeHandle::connect_cb(uv_connect_t* req, int status) {
    auto& self = PipeHandle::cast((uv_pipe_t*)req->handle);

    if (status < 0) {
      self.connect_err_cb(ErrorEvent{status});
    } else {
      self.connect_callback();
    }

    delete req;
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

  loop::loop() {
    m_loop = std::make_unique<uv_loop_t>();
    UV(uv_loop_init, m_loop.get());
    m_loop->data = this;
  }

  loop::~loop() {
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

  void loop::run() {
    UV(uv_run, m_loop.get(), UV_RUN_DEFAULT);
  }

  void loop::stop() {
    uv_stop(m_loop.get());
  }

  uv_loop_t* loop::get() const {
    return m_loop.get();
  }
  // }}}

}  // namespace eventloop

POLYBAR_NS_END

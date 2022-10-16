#include "components/eventloop.hpp"

#include <cassert>
#include <utility>

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
      case UV_PREPARE:
        static_cast<PrepareHandle*>(handle->data)->close();
        break;
      default:
        assert(false);
    }
  }

  // WriteRequest {{{
  WriteRequest::WriteRequest(cb_write&& user_cb, cb_error&& err_cb)
      : write_callback(std::move(user_cb)), write_err_cb(std::move(err_cb)) {
    get()->data = this;
  }

  WriteRequest& WriteRequest::create(cb_write&& user_cb, cb_error&& err_cb) {
    auto r = std::make_unique<WriteRequest>(std::move(user_cb), std::move(err_cb));
    return r->leak(std::move(r));
  }

  uv_write_t* WriteRequest::get() {
    return &req;
  }

  void WriteRequest::trigger(int status) {
    if (status < 0) {
      if (write_err_cb) {
        write_err_cb(ErrorEvent{status});
      }
    } else {
      if (write_callback) {
        write_callback();
      }
    }

    unleak();
  }

  WriteRequest& WriteRequest::leak(std::unique_ptr<WriteRequest> h) {
    lifetime_extender = std::move(h);
    return *lifetime_extender;
  }

  void WriteRequest::unleak() {
    reset_callbacks();
    lifetime_extender.reset();
  }

  void WriteRequest::reset_callbacks() {
    write_callback = nullptr;
    write_err_cb = nullptr;
  }
  // }}}

  // SignalHandle {{{
  void SignalHandle::init() {
    UV(uv_signal_init, loop(), get());
  }

  void SignalHandle::start(int signum, cb&& user_cb) {
    this->callback = std::move(user_cb);
    UV(uv_signal_start, get(), event_cb<SignalEvent, &SignalHandle::callback>, signum);
  }

  void SignalHandle::reset_callbacks() {
    callback = nullptr;
  }
  // }}}

  // PollHandle {{{
  void PollHandle::init(int fd) {
    UV(uv_poll_init, loop(), get(), fd);
  }

  void PollHandle::start(int events, cb&& user_cb, cb_error&& err_cb) {
    this->callback = std::move(user_cb);
    this->err_cb = std::move(err_cb);
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

  void PollHandle::reset_callbacks() {
    callback = nullptr;
    err_cb = nullptr;
  }
  // }}}

  // FSEventHandle {{{
  void FSEventHandle::init() {
    UV(uv_fs_event_init, loop(), get());
  }

  void FSEventHandle::start(const string& path, int flags, cb&& user_cb, cb_error&& err_cb) {
    this->callback = std::move(user_cb);
    this->err_cb = std::move(err_cb);
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

  void FSEventHandle::reset_callbacks() {
    callback = nullptr;
    err_cb = nullptr;
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

  void PipeHandle::connect(const string& name, cb_connect&& user_cb, cb_error&& err_cb) {
    this->connect_callback = std::move(user_cb);
    this->connect_err_cb = std::move(err_cb);
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

  void PipeHandle::reset_callbacks() {
    StreamHandle::reset_callbacks();
    connect_callback = nullptr;
    connect_err_cb = nullptr;
  }
  // }}}

  // TimerHandle {{{
  void TimerHandle::init() {
    UV(uv_timer_init, loop(), get());
  }

  void TimerHandle::start(uint64_t timeout, uint64_t repeat, cb&& user_cb) {
    this->callback = std::move(user_cb);
    UV(uv_timer_start, get(), void_event_cb<&TimerHandle::callback>, timeout, repeat);
  }

  void TimerHandle::stop() {
    UV(uv_timer_stop, get());
  }

  void TimerHandle::reset_callbacks() {
    callback = nullptr;
  }
  // }}}

  // AsyncHandle {{{
  void AsyncHandle::init(cb&& user_cb) {
    this->callback = std::move(user_cb);
    UV(uv_async_init, loop(), get(), void_event_cb<&AsyncHandle::callback>);
  }

  void AsyncHandle::send() {
    UV(uv_async_send, get());
  }

  void AsyncHandle::reset_callbacks() {
    callback = nullptr;
  }
  // }}}

  // PrepareHandle {{{
  void PrepareHandle::init() {
    UV(uv_prepare_init, loop(), get());
  }

  void PrepareHandle::start(cb&& user_cb) {
    this->callback = std::move(user_cb);
    UV(uv_prepare_start, get(), void_event_cb<&PrepareHandle::callback>);
  }

  void PrepareHandle::reset_callbacks() {
    callback = nullptr;
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

  uint64_t loop::now() const {
    return uv_now(m_loop.get());
  }

  uv_loop_t* loop::get() const {
    return m_loop.get();
  }
  // }}}

} // namespace eventloop

POLYBAR_NS_END

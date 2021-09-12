#include "components/eventloop.hpp"

#include <cassert>

POLYBAR_NS

/**
 * Closes the given wrapper.
 *
 * We have to have distinct cases for all types because we can't just cast to `UVHandleGeneric` without template
 * arguments.
 */
void close_callback(uv_handle_t* handle) {
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
SignalHandle::SignalHandle(uv_loop_t* loop, std::function<void(int)> fun) : UVHandle(fun) {
  UV(uv_signal_init, loop, handle);
}

void SignalHandle::start(int signum) {
  UV(uv_signal_start, handle, callback, signum);
}
// }}}

// PollHandle {{{
// TODO wrap callback and handle status
PollHandle::PollHandle(uv_loop_t* loop, int fd, std::function<void(int, int)> fun) : UVHandle(fun) {
  UV(uv_poll_init, loop, handle, fd);
}

void PollHandle::start(int events) {
  UV(uv_poll_start, handle, events, callback);
}
// }}}

// FSEventHandle {{{
// TODO wrap callback and handle status
FSEventHandle::FSEventHandle(uv_loop_t* loop, std::function<void(const char*, int, int)> fun) : UVHandle(fun) {
  UV(uv_fs_event_init, loop, handle);
}

void FSEventHandle::start(const string& path) {
  UV(uv_fs_event_start, handle, callback, path.c_str(), 0);
}
// }}}

// PipeHandle {{{
PipeHandle::PipeHandle(uv_loop_t* loop, std::function<void(const string)> fun)
    : UVHandleGeneric([&](ssize_t nread, const uv_buf_t* buf) { read_cb(nread, buf); }), func(fun) {
  UV(uv_pipe_init, loop, handle, false);
}

void PipeHandle::start(int fd) {
  this->fd = fd;
  UV(uv_pipe_open, handle, fd);
  UV(uv_read_start, (uv_stream_t*)handle, alloc_cb, callback);
}

void PipeHandle::read_cb(ssize_t nread, const uv_buf_t* buf) {
  auto log = logger::make();
  if (nread > 0) {
    string payload = string(buf->base, nread);
    // TODO lower logging level
    log.notice("Bytes read: %d: '%s'", nread, payload);
    func(payload);
  } else if (nread < 0) {
    if (nread != UV_EOF) {
      // TODO maybe handle this differently. exception?
      log.err("Read error: %s", uv_err_name(nread));
      uv_close((uv_handle_t*)handle, nullptr);
    } else {
      // TODO this causes constant EOFs
      start(this->fd);
    }
  }

  if (buf->base) {
    delete[] buf->base;
  }
}
// }}}

// TimerHandle {{{
TimerHandle::TimerHandle(uv_loop_t* loop, std::function<void(void)> fun) : UVHandle(fun) {
  UV(uv_timer_init, loop, handle);
}

void TimerHandle::start(uint64_t timeout, uint64_t repeat) {
  UV(uv_timer_start, handle, callback, timeout, repeat);
}
// }}}

// AsyncHandle {{{
AsyncHandle::AsyncHandle(uv_loop_t* loop, std::function<void(void)> fun) : UVHandle(fun) {
  UV(uv_async_init, loop, handle, callback);
}

void AsyncHandle::send() {
  UV(uv_async_send, handle);
}
// }}}

// eventloop {{{
static void close_walk_cb(uv_handle_t* handle, void*) {
  if (!uv_is_closing(handle)) {
    uv_close(handle, close_callback);
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

void eventloop::signal_handler(int signum, std::function<void(int)> fun) {
  m_sig_handles.emplace_back(std::make_unique<SignalHandle>(get(), fun));
  m_sig_handles.back()->start(signum);
}

void eventloop::poll_handler(int events, int fd, std::function<void(int, int)> fun) {
  m_poll_handles.emplace_back(std::make_unique<PollHandle>(get(), fd, fun));
  m_poll_handles.back()->start(events);
}

void eventloop::fs_event_handler(const string& path, std::function<void(const char*, int, int)> fun) {
  m_fs_event_handles.emplace_back(std::make_unique<FSEventHandle>(get(), fun));
  m_fs_event_handles.back()->start(path);
}

void eventloop::pipe_handle(int fd, std::function<void(const string)> fun) {
  m_pipe_handles.emplace_back(std::make_unique<PipeHandle>(get(), fun));
  m_pipe_handles.back()->start(fd);
}

void eventloop::timer_handle(uint64_t timeout, uint64_t repeat, std::function<void(void)> fun) {
  m_timer_handles.emplace_back(std::make_unique<TimerHandle>(get(), fun));
  m_timer_handles.back()->start(timeout, repeat);
}

AsyncHandle_t eventloop::async_handle(std::function<void(void)> fun) {
  m_async_handles.emplace_back(std::make_shared<AsyncHandle>(get(), fun));
  return m_async_handles.back();
}
// }}}

POLYBAR_NS_END

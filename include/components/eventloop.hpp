#pragma once

#include <uv.h>

#include <stdexcept>

#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

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

template <class H, class... Args>
struct cb_helper {
  std::function<void(Args...)> func;

  static void callback(H* context, Args... args) {
    const auto unpackedThis = static_cast<const cb_helper*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }
};

/**
 * \tparam H Type of the handle
 * \tparam I Type of the handle passed to the callback. Often the same as H, but not always (e.g. uv_read_start)
 */
template <typename H, typename I, typename... Args>
struct UVHandleGeneric {
  UVHandleGeneric(std::function<void(Args...)> fun) {
    handle = std::make_unique<H>();
    cb = cb_helper<I, Args...>{fun};
    handle->data = &cb;
  }

  std::unique_ptr<H> handle;
  cb_helper<I, Args...> cb;
};

template <typename H, typename... Args>
struct UVHandle : public UVHandleGeneric<H, H, Args...> {
  UVHandle(std::function<void(Args...)> fun) : UVHandleGeneric<H, H, Args...>(fun) {}
};

struct SignalHandle : public UVHandle<uv_signal_t, int> {
  SignalHandle(uv_loop_t* loop, std::function<void(int)> fun) : UVHandle(fun) {
    UV(uv_signal_init, loop, handle.get());
  }

  void start(int signum) {
    UV(uv_signal_start, handle.get(), cb.callback, signum);
  }
};

struct PollHandle : public UVHandle<uv_poll_t, int, int> {
  // TODO wrap callback and handle status
  PollHandle(uv_loop_t* loop, int fd, std::function<void(int, int)> fun) : UVHandle(fun) {
    UV(uv_poll_init, loop, handle.get(), fd);
  }

  void start(int events) {
    UV(uv_poll_start, handle.get(), events, &cb.callback);
  }
};

struct FSEventHandle : public UVHandle<uv_fs_event_t, const char*, int, int> {
  // TODO wrap callback and handle status
  FSEventHandle(uv_loop_t* loop, std::function<void(const char*, int, int)> fun) : UVHandle(fun) {
    UV(uv_fs_event_init, loop, handle.get());
  }

  void start(const string& path) {
    UV(uv_fs_event_start, handle.get(), &cb.callback, path.c_str(), 0);
  }
};

struct PipeHandle : public UVHandleGeneric<uv_pipe_t, uv_stream_t, ssize_t, const uv_buf_t*> {
  std::function<void(const string)> func;

  int fd;

  PipeHandle(uv_loop_t* loop, std::function<void(const string)> fun)
      : UVHandleGeneric([&](ssize_t nread, const uv_buf_t* buf) { read_cb(nread, buf); }), func(fun) {
    UV(uv_pipe_init, loop, handle.get(), false);
  }

  void start(int fd) {
    this->fd = fd;
    UV(uv_pipe_open, handle.get(), fd);
    UV(uv_read_start, (uv_stream_t*)handle.get(), alloc_cb, &cb.callback);
  }

  static void alloc_cb(uv_handle_t*, size_t, uv_buf_t* buf) {
    buf->base = new char[BUFSIZ];
    buf->len = BUFSIZ;
  }

  void read_cb(ssize_t nread, const uv_buf_t* buf) {
    auto log = logger::make();
    if (nread > 0) {
      string payload = string(buf->base, nread);
      log.notice("Bytes read: %d: '%s'", nread, payload);
      func(payload);
    } else if (nread < 0) {
      if (nread != UV_EOF) {
        log.err("Read error: %s", uv_err_name(nread));
        uv_close((uv_handle_t*)handle.get(), nullptr);
      } else {
        // TODO this causes constant EOFs
        start(this->fd);
      }
    }

    if (buf->base) {
      delete[] buf->base;
    }
  }
};

struct TimerHandle : public UVHandle<uv_timer_t> {
  TimerHandle(uv_loop_t* loop, std::function<void(void)> fun) : UVHandle(fun) {
    UV(uv_timer_init, loop, handle.get());
  }

  void start(uint64_t timeout, uint64_t repeat) {
    UV(uv_timer_start, handle.get(), &cb.callback, timeout, repeat);
  }
};

struct AsyncHandle : public UVHandle<uv_async_t> {
  AsyncHandle(uv_loop_t* loop, std::function<void(void)> fun) : UVHandle(fun) {
    UV(uv_async_init, loop, handle.get(), &cb.callback);
  }

  void send() {
    UV(uv_async_send, handle.get());
  }
};

using SignalHandle_t = std::unique_ptr<SignalHandle>;
using PollHandle_t = std::unique_ptr<PollHandle>;
using FSEventHandle_t = std::unique_ptr<FSEventHandle>;
using PipeHandle_t = std::unique_ptr<PipeHandle>;
using TimerHandle_t = std::unique_ptr<TimerHandle>;
// shared_ptr because we need a reference outside to call send
using AsyncHandle_t = std::shared_ptr<AsyncHandle>;

class eventloop {
 public:
  eventloop();
  ~eventloop();

  void run();

  void stop();

  void signal_handler(int signum, std::function<void(int)> fun) {
    m_sig_handles.emplace_back(std::make_unique<SignalHandle>(get(), fun));
    m_sig_handles.back()->start(signum);
  }

  void poll_handler(int events, int fd, std::function<void(int, int)> fun) {
    m_poll_handles.emplace_back(std::make_unique<PollHandle>(get(), fd, fun));
    m_poll_handles.back()->start(events);
  }

  void fs_event_handler(const string& path, std::function<void(const char*, int, int)> fun) {
    m_fs_event_handles.emplace_back(std::make_unique<FSEventHandle>(get(), fun));
    m_fs_event_handles.back()->start(path);
  }

  void pipe_handle(int fd, std::function<void(const string)> fun) {
    m_pipe_handles.emplace_back(std::make_unique<PipeHandle>(get(), fun));
    m_pipe_handles.back()->start(fd);
  }

  void timer_handle(uint64_t timeout, uint64_t repeat, std::function<void(void)> fun) {
    m_timer_handles.emplace_back(std::make_unique<TimerHandle>(get(), fun));
    // Trigger a single screenshot after 3 seconds
    m_timer_handles.back()->start(timeout, repeat);
  }

  AsyncHandle_t async_handle(std::function<void(void)> fun) {
    m_async_handles.emplace_back(std::make_shared<AsyncHandle>(get(), fun));
    return m_async_handles.back();
  }

 protected:
  uv_loop_t* get() const {
    return m_loop.get();
  }

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};

  vector<SignalHandle_t> m_sig_handles;
  vector<PollHandle_t> m_poll_handles;
  vector<FSEventHandle_t> m_fs_event_handles;
  vector<PipeHandle_t> m_pipe_handles;
  vector<TimerHandle_t> m_timer_handles;
  vector<AsyncHandle_t> m_async_handles;
};

POLYBAR_NS_END

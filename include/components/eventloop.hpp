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
#define UV(fun, ...)                                                                \
  do {                                                                              \
    int res = fun(__VA_ARGS__);                                                     \
    if (res < 0) {                                                                  \
      throw std::runtime_error("libuv error for '" #fun "': "s + uv_strerror(res)); \
    }                                                                               \
  } while (0);

template <class H, class... Args>
struct cb_helper {
  std::function<void(Args...)> func;

  static void callback(H* context, Args... args) {
    const auto unpackedThis = static_cast<const cb_helper*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }
};

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
  PipeHandle(uv_loop_t* loop, std::function<void(const string)> fun)
      : UVHandleGeneric([&](ssize_t nread, const uv_buf_t* buf) {
        if (nread > 0) {
          string payload = string(buf->base, nread);
          logger::make().notice("Bytes read: %d: '%s'", nread, payload);
          fun(std::move(payload));
        } else if (nread < 0) {
          if (nread != UV_EOF) {
            logger::make().err("Read error: %s", uv_err_name(nread));
            uv_close((uv_handle_t*)handle.get(), nullptr);
          } else {
            UV(
                uv_read_start, (uv_stream_t*)handle.get(),
                [](uv_handle_t*, size_t, uv_buf_t* buf) {
                  buf->base = new char[BUFSIZ];
                  buf->len = BUFSIZ;
                },
                &cb.callback);
          }
        }

        if (buf->base) {
          delete[] buf->base;
        }
      }) {
    UV(uv_pipe_init, loop, handle.get(), false);
  }

  void start(int fd) {
    UV(uv_pipe_open, handle.get(), fd);
    UV(
        uv_read_start, (uv_stream_t*)handle.get(),
        [](uv_handle_t*, size_t, uv_buf_t* buf) {
          buf->base = new char[BUFSIZ];
          buf->len = BUFSIZ;
        },
        &cb.callback);
  }
};

class eventloop {
 public:
  eventloop();
  ~eventloop();

  void run();

  void stop();

  /**
   * TODO make protected
   */
  uv_loop_t* get() const {
    return m_loop.get();
  }

  void signal_handler(int signum, std::function<void(int)> fun) {
    auto handle = std::make_unique<SignalHandle>(get(), fun);
    handle->start(signum);
    m_sig_handles.push_back(std::move(handle));
  }

  void poll_handler(int events, int fd, std::function<void(int, int)> fun) {
    auto handle = std::make_unique<PollHandle>(get(), fd, fun);
    handle->start(events);
    m_poll_handles.push_back(std::move(handle));
  }

  void fs_event_handler(const string& path, std::function<void(const char*, int, int)> fun) {
    auto handle = std::make_unique<FSEventHandle>(get(), fun);
    handle->start(path);
    m_fs_event_handles.push_back(std::move(handle));
  }

 protected:
  template <typename H, typename T, typename... Args, void (T::*F)(Args...)>
  static void generic_cb(H* handle, Args&&... args) {
    (static_cast<T*>(handle->data).*F)(std::forward<Args>(args)...);
  }

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};

  vector<std::unique_ptr<SignalHandle>> m_sig_handles;
  vector<std::unique_ptr<PollHandle>> m_poll_handles;
  vector<std::unique_ptr<FSEventHandle>> m_fs_event_handles;
};

POLYBAR_NS_END

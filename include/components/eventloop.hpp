#pragma once

#include <uv.h>

#include <stdexcept>

#include "common.hpp"

POLYBAR_NS

/**
 * Runs any libuv function with an integer error code return value and throws an
 * exception on error.
 */
#define UV(fun, ...)                                                              \
  int res = fun(__VA_ARGS__);                                                     \
  if (res < 0) {                                                                  \
    throw std::runtime_error("libuv error for '" #fun "': "s + uv_strerror(res)); \
  }

template <class H, class... Args>
struct cb_helper {
  std::function<void(Args...)> func;

  static void callback(H* context, Args... args) {
    const auto unpackedThis = static_cast<const cb_helper*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }
};

template <typename H, typename... Args>
struct UVHandle {
  UVHandle(std::function<void(Args...)> fun) {
    handle = std::make_unique<H>();
    cb = cb_helper<H, Args...>{fun};
    handle->data = &cb;
  }

  std::unique_ptr<H> handle;
  cb_helper<H, Args...> cb;
};

struct SignalHandle : public UVHandle<uv_signal_t, int> {
  SignalHandle(uv_loop_t* loop, std::function<void(int)> fun) : UVHandle(fun) {
    UV(uv_signal_init, loop, handle.get());
  };
};

struct PollHandle : public UVHandle<uv_poll_t, int, int> {
  // TODO wrap callback and handle negative status
  PollHandle(uv_loop_t* loop, int fd, std::function<void(int, int)> fun) : UVHandle(fun) {
    UV(uv_poll_init, loop, handle.get(), fd);
  };
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
    UV(uv_signal_start, handle->handle.get(), &handle->cb.callback, signum);
    m_sig_handles.push_back(std::move(handle));
  }

  void poll_handler(int events, int fd, std::function<void(int, int)> fun) {
    auto handle = std::make_unique<PollHandle>(get(), fd, fun);
    UV(uv_poll_start, handle->handle.get(), events, &handle->cb.callback);
    m_poll_handles.push_back(std::move(handle));
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
};

POLYBAR_NS_END

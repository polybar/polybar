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

void close_callback(uv_handle_t*);

/**
 * \tparam H Type of the handle
 * \tparam I Type of the handle passed to the callback. Often the same as H, but not always (e.g. uv_read_start)
 */
template <typename H, typename I, typename... Args>
struct UVHandleGeneric {
  UVHandleGeneric(function<void(Args...)> fun) {
    handle = new H;
    handle->data = this;
    this->func = fun;
  }

  ~UVHandleGeneric() {
    close();
  }

  void close() {
    if (handle && !uv_is_closing((uv_handle_t*)handle)) {
      uv_close((uv_handle_t*)handle, close_callback);
    }
  }

  void cleanup_resources() {
    if (handle) {
      delete handle;
      handle = nullptr;
    }
  }

  static void callback(I* context, Args... args) {
    const auto unpackedThis = static_cast<const UVHandleGeneric*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }

  H* handle;
  function<void(Args...)> func;
};

template <typename H, typename... Args>
struct UVHandle : public UVHandleGeneric<H, H, Args...> {
  UVHandle(function<void(Args...)> fun) : UVHandleGeneric<H, H, Args...>(fun) {}
};

struct SignalHandle : public UVHandle<uv_signal_t, int> {
  SignalHandle(uv_loop_t* loop, function<void(int)> fun);
  void start(int signum);
};

struct PollHandle : public UVHandle<uv_poll_t, int, int> {
  PollHandle(uv_loop_t* loop, int fd, function<void(uv_poll_event)> fun, function<void(int)> err_cb);
  void start(int events);
  void poll_cb(int status, int events);

  function<void(uv_poll_event)> func;
  function<void(int)> err_cb;
};

struct FSEventHandle : public UVHandle<uv_fs_event_t, const char*, int, int> {
  FSEventHandle(uv_loop_t* loop, function<void(const char*, uv_fs_event)> fun, function<void(int)> err_cb);
  void start(const string& path);
  void fs_event_cb(const char* path, int events, int status);

  function<void(const char*, uv_fs_event)> func;
  function<void(int)> err_cb;
};

struct PipeHandle : public UVHandleGeneric<uv_pipe_t, uv_stream_t, ssize_t, const uv_buf_t*> {
  PipeHandle(
      uv_loop_t* loop, function<void(const string)> fun, function<void(void)> eof_cb, function<void(int)> err_cb);
  void start(int fd);
  void read_cb(ssize_t nread, const uv_buf_t* buf);

  function<void(const string)> func;
  function<void(void)> eof_cb;
  function<void(int)> err_cb;
  int fd;
};

struct TimerHandle : public UVHandle<uv_timer_t> {
  TimerHandle(uv_loop_t* loop, function<void(void)> fun);
  void start(uint64_t timeout, uint64_t repeat);
};

struct AsyncHandle : public UVHandle<uv_async_t> {
  AsyncHandle(uv_loop_t* loop, function<void(void)> fun);
  void send();
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
  void signal_handle(int signum, function<void(int)> fun);
  void poll_handle(int events, int fd, function<void(uv_poll_event)> fun, function<void(int)> err_cb);
  void fs_event_handle(const string& path, function<void(const char*, uv_fs_event)> fun, function<void(int)> err_cb);
  void pipe_handle(int fd, function<void(const string)> fun, function<void(void)> eof_cb, function<void(int)> err_cb);
  void timer_handle(uint64_t timeout, uint64_t repeat, function<void(void)> fun);
  AsyncHandle_t async_handle(function<void(void)> fun);

 protected:
  uv_loop_t* get() const;

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

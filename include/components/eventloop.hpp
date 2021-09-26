#pragma once

#include <uv.h>

#include <stdexcept>
#include <unordered_set>

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

  uv_loop_t* loop() const {
    return handle->loop;
  }

  void close() {
    if (!is_closing()) {
      uv_close((uv_handle_t*)handle, close_callback);
    }
  }

  bool is_closing() {
    return !handle || uv_is_closing((uv_handle_t*)handle);
  }

  bool is_active() {
    return uv_is_active((uv_handle_t*)handle) != 0;
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

  H* handle{nullptr};
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

  void start();
  void read_cb(ssize_t nread, const uv_buf_t* buf);

  /**
   * Callback for receiving data
   */
  function<void(const string)> func;
  /**
   * Callback for receiving EOF.
   *
   * Called after the associated handle has been closed.
   */
  function<void(void)> eof_cb;

  /**
   * Called if an error occurs.
   */
  function<void(int)> err_cb;
};

struct SocketHandle : public PipeHandle {
  SocketHandle(uv_loop_t* loop, const string& sock_path, function<void(const string)> fun, function<void(void)> eof_cb,
      function<void(int)> err_cb);

  void start();

  std::unordered_set<shared_ptr<PipeHandle>> clients;
};

struct NamedPipeHandle : public PipeHandle {
  NamedPipeHandle(uv_loop_t* loop, const string& path, function<void(const string)> fun, function<void(void)> eof_cb,
      function<void(int)> err_cb);

  void start();
  void close_cb(void);

  function<void(void)> eof_cb;

  int fd;
  string path;
};

struct TimerHandle : public UVHandle<uv_timer_t> {
  TimerHandle(uv_loop_t* loop, function<void(void)> fun);
  void start(uint64_t timeout, uint64_t repeat, function<void(void)> new_cb = function<void(void)>(nullptr));
  void stop();
};

struct AsyncHandle : public UVHandle<uv_async_t> {
  AsyncHandle(uv_loop_t* loop, function<void(void)> fun);
  void send();
};

using SignalHandle_t = std::unique_ptr<SignalHandle>;
using PollHandle_t = std::unique_ptr<PollHandle>;
using FSEventHandle_t = std::unique_ptr<FSEventHandle>;
using NamedPipeHandle_t = std::unique_ptr<NamedPipeHandle>;
// shared_ptr because we also return the pointer in order to call methods on it
using TimerHandle_t = std::shared_ptr<TimerHandle>;
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
  void named_pipe_handle(
      const string& path, function<void(const string)> fun, function<void(void)> eof_cb, function<void(int)> err_cb);
  TimerHandle_t timer_handle(function<void(void)> fun);
  AsyncHandle_t async_handle(function<void(void)> fun);

 protected:
  uv_loop_t* get() const;

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};

  vector<SignalHandle_t> m_sig_handles;
  vector<PollHandle_t> m_poll_handles;
  vector<FSEventHandle_t> m_fs_event_handles;
  vector<NamedPipeHandle_t> m_named_pipe_handles;
  vector<TimerHandle_t> m_timer_handles;
  vector<AsyncHandle_t> m_async_handles;
};

POLYBAR_NS_END

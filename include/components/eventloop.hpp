#pragma once

#include <uv.h>

#include <stdexcept>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace eventloop {

  using cb_status = std::function<void(int)>;

  /**
   * Callback variant for uv_read_cb in the successful case.
   */
  using cb_read = function<void(const char*, size_t)>;

  using cb_void = function<void(void)>;

  template <typename Self, typename H>
  class Handle : public non_copyable_mixin {
   public:
    Handle(uv_loop_t* l) : uv_loop(l) {
      get()->data = this;
    }

    void leak(std::unique_ptr<Self> h) {
      lifetime_extender = std::move(h);
    }

    void unleak() {
      lifetime_extender.reset();
    }

    void close() {
      if (!is_closing()) {
        uv_close((uv_handle_t*)get(), [](uv_handle_t* handle) { close_callback(*static_cast<Self*>(handle->data)); });
      }
    }

    bool is_closing() const {
      return uv_is_closing((const uv_handle_t*)get());
    }

    bool is_active() {
      return uv_is_active((const uv_handle_t*)get()) != 0;
    }

   protected:
    /**
     * Generic callback function that can be used for all uv handle callbacks.
     *
     * \tparam Event Event class/struct. Must have a constructor that takes all arguments passed to the uv callback,
     * except for the handle argument.
     * \tparam Member Pointer to class member where callback function is stored
     * \tparam Args Additional arguments in the uv callback. Inferred by the compiler
     */
    template <typename Event, std::function<void(const Event&)> Self::*Member, typename... Args>
    static void event_cb(H* handle, Args... args) {
      Self& This = *static_cast<Self*>(handle->data);
      (This.*Member)(Event{std::forward<Args>(args)...});
    }

    static Self& cast(H* handle) {
      return *static_cast<Self*>(handle->data);
    }

    H* get() {
      return &uv_handle;
    }

    const H* get() const {
      return &uv_handle;
    }

    uv_loop_t* loop() const {
      return uv_loop;
    }

    // TODO allow user callback
    static void close_callback(Self& self) {
      self.unleak();
    }

   private:
    H uv_handle;
    uv_loop_t* uv_loop;

    /**
     * The handle stores the unique_ptr to itself so that it effectively leaks memory.
     *
     * This saves us from having to guarantee that the handle's lifetime extends to at least after it is closed.
     *
     * Once the handle is closed, either explicitly or by walking all handles when the loop shuts down, this reference
     * is reset and the object is explicitly destroyed.
     */
    std::unique_ptr<Self> lifetime_extender;
  };

  struct ErrorEvent {
    int status;
  };

  using cb_error = std::function<void(const ErrorEvent&)>;

  struct SignalEvent {
    int signum;
  };

  class SignalHandle : public Handle<SignalHandle, uv_signal_t> {
   public:
    using Handle::Handle;
    using cb = std::function<void(const SignalEvent&)>;

    void init();
    void start(int signum, cb user_cb);

   private:
    cb callback;
  };

  struct PollEvent {
    uv_poll_event event;
  };

  class PollHandle : public Handle<PollHandle, uv_poll_t> {
   public:
    using Handle::Handle;
    using cb = std::function<void(const PollEvent&)>;

    void init(int fd);
    void start(int events, cb user_cb, cb_error err_cb);
    static void poll_callback(uv_poll_t*, int status, int events);

   private:
    cb callback;
    cb_error err_cb;
  };

  struct FSEvent {
    const char* path;
    uv_fs_event event;
  };

  class FSEventHandle : public Handle<FSEventHandle, uv_fs_event_t> {
   public:
    using Handle::Handle;
    using cb = std::function<void(const FSEvent&)>;

    void init();
    void start(const string& path, int flags, cb user_cb, cb_error err_cb);
    static void fs_event_callback(uv_fs_event_t*, const char* path, int events, int status);

   private:
    cb callback;
    cb_error err_cb;
  };

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
        uv_close((uv_handle_t*)handle,
            [](uv_handle_t* handle) { cleanup_resources(*static_cast<UVHandleGeneric*>(handle->data)); });
      }
    }

    bool is_closing() const {
      return !handle || uv_is_closing((const uv_handle_t*)handle);
    }

    bool is_active() {
      return uv_is_active((uv_handle_t*)handle) != 0;
    }

    static void cleanup_resources(UVHandleGeneric& self) {
      if (self.handle) {
        delete self.handle;
        self.handle = nullptr;
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

  struct PipeHandle : public UVHandleGeneric<uv_pipe_t, uv_stream_t, ssize_t, const uv_buf_t*> {
    PipeHandle(uv_loop_t* loop, cb_read fun = cb_read{nullptr}, cb_void eof_cb = cb_void{nullptr},
        cb_status err_cb = cb_status{nullptr});

    void start(cb_read fun, cb_void eof_cb, cb_status err_cb);
    void read_cb(ssize_t nread, const uv_buf_t* buf);

    /**
     * Callback for receiving data
     */
    cb_read func;
    /**
     * Callback for receiving EOF.
     *
     * Called after the associated handle has been closed.
     */
    cb_void eof_cb;

    /**
     * Called if an error occurs.
     */
    cb_status err_cb;
  };

  struct NamedPipeHandle : public PipeHandle {
    NamedPipeHandle(uv_loop_t* loop, const string& path, cb_read fun, cb_void eof_cb, cb_status err_cb);

    void start();
    void close_cb(void);

    cb_void eof_cb;

    int fd;
    string path;
  };

  struct TimerHandle : public UVHandle<uv_timer_t> {
    TimerHandle(uv_loop_t* loop, cb_void fun);
    void start(uint64_t timeout, uint64_t repeat, cb_void new_cb = cb_void{nullptr});
    void stop();
  };

  struct AsyncHandle : public UVHandle<uv_async_t> {
    AsyncHandle(uv_loop_t* loop, cb_void fun);
    void send();
  };

  struct SocketHandle : public UVHandleGeneric<uv_pipe_t, uv_stream_t, int> {
    SocketHandle(uv_loop_t* loop, const string& sock_path, cb_void connection_cb, cb_status err_cb);

    void listen(int backlog);

    void on_connection(int status);

    void accept(PipeHandle& pipe);

    string path;

    cb_void connection_cb;
    /**
     * Called if an error occurs.
     */
    cb_status err_cb;
  };

  using NamedPipeHandle_t = std::unique_ptr<NamedPipeHandle>;
  // shared_ptr because we also return the pointer in order to call methods on it
  using PipeHandle_t = std::shared_ptr<PipeHandle>;
  using TimerHandle_t = std::shared_ptr<TimerHandle>;
  using AsyncHandle_t = std::shared_ptr<AsyncHandle>;
  using SocketHandle_t = std::shared_ptr<SocketHandle>;

  class eventloop {
   public:
    eventloop();
    ~eventloop();
    void run();
    void stop();
    void signal_handle(int signum, SignalHandle::cb fun);
    void fs_event_handle(const string& path, function<void(const char*, uv_fs_event)> fun, cb_status err_cb);
    void named_pipe_handle(const string& path, cb_read fun, cb_void eof_cb, cb_status err_cb);
    TimerHandle_t timer_handle(cb_void fun);
    AsyncHandle_t async_handle(cb_void fun);
    SocketHandle_t socket_handle(const string& path, int backlog, cb_void connection_cb, cb_status err_cb);
    PipeHandle_t pipe_handle();

    template <typename H, typename... Args>
    H& handle(Args... args) {
      auto ptr = make_unique<H>(get());
      H& ref = *ptr;
      ref.init(std::forward<Args>(args)...);
      ref.leak(std::move(ptr));
      return ref;
    }

   protected:
    uv_loop_t* get() const;

   private:
    std::unique_ptr<uv_loop_t> m_loop{nullptr};

    vector<NamedPipeHandle_t> m_named_pipe_handles;
    vector<TimerHandle_t> m_timer_handles;
    vector<AsyncHandle_t> m_async_handles;
    vector<SocketHandle_t> m_socket_handles;
  };

};  // namespace eventloop

POLYBAR_NS_END

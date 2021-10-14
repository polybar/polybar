#pragma once

#include <uv.h>

#include <stdexcept>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace eventloop {
  using cb_void = function<void(void)>;

  template <typename Event>
  using cb_event = std::function<void(const Event&)>;

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

    H* raw() {
      return get();
    }

    const H* raw() const {
      return get();
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
    template <typename Event, cb_event<Event> Self::*Member, typename... Args>
    static void event_cb(H* handle, Args... args) {
      Self& This = *static_cast<Self*>(handle->data);
      (This.*Member)(Event{std::forward<Args>(args)...});
    }

    /**
     * Same as event_cb except that no event is constructed.
     */
    template <cb_void Self::*Member>
    static void void_event_cb(H* handle) {
      Self& This = *static_cast<Self*>(handle->data);
      (This.*Member)();
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

  using cb_error = cb_event<ErrorEvent>;

  struct SignalEvent {
    int signum;
  };

  class SignalHandle : public Handle<SignalHandle, uv_signal_t> {
   public:
    using Handle::Handle;
    using cb = cb_event<SignalEvent>;

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
    using cb = cb_event<PollEvent>;

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
    using cb = cb_event<FSEvent>;

    void init();
    void start(const string& path, int flags, cb user_cb, cb_error err_cb);
    static void fs_event_callback(uv_fs_event_t*, const char* path, int events, int status);

   private:
    cb callback;
    cb_error err_cb;
  };

  class TimerHandle : public Handle<TimerHandle, uv_timer_t> {
   public:
    using Handle::Handle;
    using cb = cb_void;

    void init();
    void start(uint64_t timeout, uint64_t repeat, cb user_cb);
    void stop();

   private:
    cb callback;
  };

  class AsyncHandle : public Handle<AsyncHandle, uv_async_t> {
   public:
    using Handle::Handle;
    using cb = cb_void;

    void init(cb user_cb);
    void send();

   private:
    cb callback;
  };

  struct ReadEvent {
    const char* data;
    size_t len;
  };

  class PipeHandle : public Handle<PipeHandle, uv_pipe_t> {
   public:
    using Handle::Handle;
    using cb_read = cb_event<ReadEvent>;
    using cb_read_eof = cb_void;

    void init(bool ipc = false);
    void open(int fd);
    void read_start(cb_read user_cb, cb_read_eof eof_cb, cb_error err_cb);
    static void read_cb(uv_stream_t*, ssize_t nread, const uv_buf_t* buf);

   private:
    /**
     * Callback for receiving data
     */
    cb_read read_callback;

    /**
     * Callback for receiving EOF.
     *
     * Called after the associated handle has been closed.
     */
    cb_read_eof read_eof_cb;

    /**
     * Called if an error occurs.
     */
    cb_error read_err_cb;
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

  struct SocketHandle : public UVHandleGeneric<uv_pipe_t, uv_stream_t, int> {
    SocketHandle(uv_loop_t* loop, const string& sock_path, cb_void connection_cb, std::function<void(int)> err_cb);

    void listen(int backlog);

    void on_connection(int status);

    void accept(PipeHandle& pipe);

    string path;

    cb_void connection_cb;
    /**
     * Called if an error occurs.
     */
    std::function<void(int)> err_cb;
  };

  // shared_ptr because we also return the pointer in order to call methods on it
  using PipeHandle_t = std::shared_ptr<PipeHandle>;
  using SocketHandle_t = std::shared_ptr<SocketHandle>;

  class eventloop {
   public:
    eventloop();
    ~eventloop();
    void run();
    void stop();
    SocketHandle_t socket_handle(
        const string& path, int backlog, cb_void connection_cb, std::function<void(int)> err_cb);

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

    vector<SocketHandle_t> m_socket_handles;
  };

};  // namespace eventloop

POLYBAR_NS_END

#pragma once

#include <uv.h>

#include <stdexcept>
#include <utility>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace eventloop {
/**
 * Runs any libuv function with an integer error code return value and throws an
 * exception on error.
 */
#define UV(fun, ...)                                                                                    \
  do {                                                                                                  \
    int res = fun(__VA_ARGS__);                                                                         \
    if (res < 0) {                                                                                      \
      throw std::runtime_error(__FILE__ ":"s + std::to_string(__LINE__) +                               \
                               ": libuv error for '" #fun "(" #__VA_ARGS__ ")': "s + uv_strerror(res)); \
    }                                                                                                   \
  } while (0);

  using cb_void = function<void(void)>;

  template <typename Event>
  using cb_event = std::function<void(const Event&)>;

  template <typename Self, typename H>
  class Handle : public non_copyable_mixin, public non_movable_mixin {
   public:
    Handle(uv_loop_t* l) : uv_loop(l) {
      get()->data = this;
    }

    void leak(std::shared_ptr<Self> h) {
      lifetime_extender = std::move(h);
    }

    void unleak() {
      reset_callbacks();
      lifetime_extender.reset();
    }

    H* raw() {
      return get();
    }

    const H* raw() const {
      return get();
    }

    /**
     * Close this handle and free associated memory.
     *
     * After this function returns, any reference to this object should be considered invalid.
     */
    void close() {
      if (!is_closing()) {
        uv_close((uv_handle_t*)get(), [](uv_handle_t* handle) { close_callback(*static_cast<Self*>(handle->data)); });
      }
    }

    bool is_closing() const {
      return uv_is_closing(this->template get<uv_handle_t>());
    }

    bool is_active() {
      return uv_is_active(this->template get<uv_handle_t>()) != 0;
    }

   protected:
    ~Handle() = default;

    /**
     * Resets all callbacks stored in the handle as part of closing the handle.
     *
     * This releases any lambda captures, breaking possible cyclic dependencies in shared_ptr.
     */
    virtual void reset_callbacks() = 0;

    /**
     * Generic callback function that can be used for all uv handle callbacks.
     *
     * @tparam Event Event class/struct. Must have a constructor that takes all arguments passed to the uv callback,
     * except for the handle argument.
     * @tparam Member Pointer to class member where callback function is stored
     * @tparam Args Additional arguments in the uv callback. Inferred by the compiler
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

    template <typename T = H>
    T* get() {
      return reinterpret_cast<T*>(&uv_handle);
    }

    template <typename T = H>
    const T* get() const {
      return reinterpret_cast<const T*>(&uv_handle);
    }

    uv_loop_t* loop() const {
      return uv_loop;
    }

    static void close_callback(Self& self) {
      self.unleak();
    }

    static void alloc_callback(uv_handle_t*, size_t, uv_buf_t* buf) {
      buf->base = new char[BUFSIZ];
      buf->len = BUFSIZ;
    }

   private:
    H uv_handle;
    uv_loop_t* uv_loop;

    /**
     * The handle stores the shared_ptr to itself so that it effectively leaks memory.
     *
     * This saves us from having to guarantee that the handle's lifetime extends to at least after it is closed.
     *
     * Once the handle is closed, either explicitly or by walking all handles when the loop shuts down, this reference
     * is reset and the object is explicitly destroyed.
     */
    std::shared_ptr<Self> lifetime_extender;
  };

  struct ErrorEvent {
    int status;
  };

  using cb_error = cb_event<ErrorEvent>;

  class WriteRequest : public non_copyable_mixin, public non_movable_mixin {
   public:
    using cb_write = cb_void;

    WriteRequest(cb_write&& user_cb, cb_error&& err_cb);

    static WriteRequest& create(cb_write&& user_cb, cb_error&& err_cb);

    uv_write_t* get();

    /**
     * Trigger the write callback.
     *
     * After that, this object is destroyed.
     */
    void trigger(int status);

   protected:
    WriteRequest& leak(std::unique_ptr<WriteRequest> h);

    void unleak();

    void reset_callbacks();

   private:
    uv_write_t req{};

    cb_write write_callback;
    cb_error write_err_cb;

    /**
     * The handle stores the unique_ptr to itself so that it effectively leaks memory.
     *
     * This means that each instance manages its own lifetime.
     */
    std::unique_ptr<WriteRequest> lifetime_extender;
  };

  struct SignalEvent {
    int signum;
  };

  class SignalHandle final : public Handle<SignalHandle, uv_signal_t> {
   public:
    using Handle::Handle;
    using cb = cb_event<SignalEvent>;

    void init();
    void start(int signum, cb&& user_cb);

   protected:
    void reset_callbacks() override;

   private:
    cb callback;
  };

  struct PollEvent {
    uv_poll_event event;
  };

  class PollHandle final : public Handle<PollHandle, uv_poll_t> {
   public:
    using Handle::Handle;
    using cb = cb_event<PollEvent>;

    void init(int fd);
    void start(int events, cb&& user_cb, cb_error&& err_cb);
    static void poll_callback(uv_poll_t*, int status, int events);

   protected:
    void reset_callbacks() override;

   private:
    cb callback;
    cb_error err_cb;
  };

  struct FSEvent {
    const char* path;
    uv_fs_event event;
  };

  class FSEventHandle final : public Handle<FSEventHandle, uv_fs_event_t> {
   public:
    using Handle::Handle;
    using cb = cb_event<FSEvent>;

    void init();
    void start(const string& path, int flags, cb&& user_cb, cb_error&& err_cb);
    static void fs_event_callback(uv_fs_event_t*, const char* path, int events, int status);

   protected:
    void reset_callbacks() override;

   private:
    cb callback;
    cb_error err_cb;
  };

  class TimerHandle final : public Handle<TimerHandle, uv_timer_t> {
   public:
    using Handle::Handle;
    using cb = cb_void;

    void init();
    void start(uint64_t timeout, uint64_t repeat, cb&& user_cb);
    void stop();

   protected:
    void reset_callbacks() override;

   private:
    cb callback;
  };

  class AsyncHandle final : public Handle<AsyncHandle, uv_async_t> {
   public:
    using Handle::Handle;
    using cb = cb_void;

    void init(cb&& user_cb);
    void send();

   protected:
    void reset_callbacks() override;

   private:
    cb callback;
  };

  struct ReadEvent {
    const char* data;
    size_t len;
  };

  template <typename Self, typename H>
  class StreamHandle : public Handle<Self, H> {
   public:
    using Handle<Self, H>::Handle;
    using cb_read = cb_event<ReadEvent>;
    using cb_read_eof = cb_void;
    using cb_connection = cb_void;

    void listen(int backlog, cb_connection&& user_cb, cb_error&& err_cb) {
      this->connection_callback = std::move(user_cb);
      this->connection_err_cb = std::move(err_cb);
      UV(uv_listen, this->template get<uv_stream_t>(), backlog, connection_cb);
    };

    static void connection_cb(uv_stream_t* server, int status) {
      auto& self = Self::cast((H*)server);

      if (status == 0) {
        self.connection_callback();
      } else {
        self.connection_err_cb(ErrorEvent{status});
      }
    }

    template <typename ClientSelf, typename ClientH>
    void accept(StreamHandle<ClientSelf, ClientH>& client) {
      UV(uv_accept, this->template get<uv_stream_t>(), client.template get<uv_stream_t>());
    }

    void read_start(cb_read&& fun, cb_void&& eof_cb, cb_error&& err_cb) {
      this->read_callback = std::move(fun);
      this->read_eof_cb = std::move(eof_cb);
      this->read_err_cb = std::move(err_cb);
      UV(uv_read_start, this->template get<uv_stream_t>(), &this->alloc_callback, &read_cb);
    };

    static void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
      auto& self = Self::cast((H*)handle);
      /*
       * Wrap pointer so that it gets automatically freed once the function returns (even with exceptions)
       */
      auto buf_ptr = unique_ptr<char[]>(buf->base);
      if (nread > 0) {
        self.read_callback(ReadEvent{buf->base, (size_t)nread});
      } else if (nread < 0) {
        if (nread != UV_EOF) {
          self.read_err_cb(ErrorEvent{(int)nread});
        } else {
          self.read_eof_cb();
        }
      }
    };

    void write(const vector<uint8_t>& data, WriteRequest::cb_write&& user_cb = {}, cb_error&& err_cb = {}) {
      WriteRequest& req = WriteRequest::create(std::move(user_cb), std::move(err_cb));

      uv_buf_t buf{(char*)data.data(), data.size()};

      UV(uv_write, req.get(), this->template get<uv_stream_t>(), &buf, 1,
          [](uv_write_t* r, int status) { static_cast<WriteRequest*>(r->data)->trigger(status); });
    }

   protected:
    ~StreamHandle() = default;

    void reset_callbacks() override {
      read_callback = nullptr;
      read_eof_cb = nullptr;
      read_err_cb = nullptr;
      connection_callback = nullptr;
      connection_err_cb = nullptr;
    }

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

    cb_connection connection_callback;
    cb_error connection_err_cb;
  };

  class PipeHandle final : public StreamHandle<PipeHandle, uv_pipe_t> {
   public:
    using StreamHandle::StreamHandle;
    using cb_connect = cb_void;

    void init(bool ipc = false);
    void open(int fd);

    void bind(const string& path);

    void connect(const string& name, cb_connect&& user_cb, cb_error&& err_cb);

   protected:
    void reset_callbacks() override;

   private:
    static void connect_cb(uv_connect_t* req, int status);

    cb_error connect_err_cb;
    cb_connect connect_callback;
  };

  class PrepareHandle final : public Handle<PrepareHandle, uv_prepare_t> {
   public:
    using Handle::Handle;
    using cb = cb_void;

    void init();
    void start(cb&& user_cb);

   protected:
    void reset_callbacks() override;

   private:
    static void connect_cb(uv_connect_t* req, int status);

    cb callback;
  };

  using signal_handle_t = shared_ptr<SignalHandle>;
  using poll_handle_t = shared_ptr<PollHandle>;
  using fs_event_handle_t = shared_ptr<FSEventHandle>;
  using timer_handle_t = shared_ptr<TimerHandle>;
  using async_handle_t = shared_ptr<AsyncHandle>;
  using pipe_handle_t = shared_ptr<PipeHandle>;
  using prepare_handle_t = shared_ptr<PrepareHandle>;

  class loop : public non_copyable_mixin, public non_movable_mixin {
   public:
    loop();
    ~loop();
    void run();
    void stop();
    uint64_t now() const;

    template <typename H, typename... Args>
    shared_ptr<H> handle(Args&&... args) {
      auto ptr = make_shared<H>(get());
      ptr->init(std::forward<Args>(args)...);
      ptr->leak(ptr);
      return ptr;
    }

    uv_loop_t* get() const;

   private:
    std::unique_ptr<uv_loop_t> m_loop{nullptr};
  };

} // namespace eventloop

POLYBAR_NS_END

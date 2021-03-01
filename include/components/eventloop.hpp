#pragma once

#include <uv.h>

#include "common.hpp"

POLYBAR_NS

template <class H, class... Args>
struct cb_helper {
  std::function<void(Args...)> func;

  static void callback(H* context, Args... args) {
    const auto unpackedThis = static_cast<const cb_helper*>(context->data);
    return unpackedThis->func(std::forward<Args>(args)...);
  }
};

struct SignalHandle {
  SignalHandle(uv_loop_t* loop, std::function<void(int)> fun) {
    handle = std::make_unique<uv_signal_t>();
    // TODO handle return value
    uv_signal_init(loop, handle.get());
    cb = cb_helper<uv_signal_t, int>{fun};

    handle->data = &cb;
  };

  std::unique_ptr<uv_signal_t> handle;
  cb_helper<uv_signal_t, int> cb;
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
    uv_signal_start(handle->handle.get(), &handle->cb.callback, signum);
    m_sig_handles.push_back(std::move(handle));
  }

 protected:
  template <typename H, typename T, typename... Args, void (T::*F)(Args...)>
  static void generic_cb(H* handle, Args&&... args) {
    (static_cast<T*>(handle->data).*F)(std::forward<Args>(args)...);
  }

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};

  vector<std::unique_ptr<SignalHandle>> m_sig_handles;
};

POLYBAR_NS_END

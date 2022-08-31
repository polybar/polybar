#pragma once

#include <mutex>
#include <thread>

#include "common.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

using namespace std::chrono_literals;
namespace this_thread = std::this_thread;

using std::mutex;
using std::thread;

/**
 * Types wrapped in this type can be used as locks (e.g. for lock_guard).
 */
template <typename T>
class mutex_wrapper : public T {
 public:
  template <typename... Args>
  explicit mutex_wrapper(Args&&... args) : T(forward<Args>(args)...) {}

  void lock() const noexcept {
    m_mtx.lock();
  }
  void unlock() const noexcept {
    m_mtx.unlock();
  };

 private:
  mutable mutex m_mtx;
};

namespace concurrency_util {
  size_t thread_id(const thread::id id);
}

POLYBAR_NS_END

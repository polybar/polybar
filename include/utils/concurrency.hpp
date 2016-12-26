#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "common.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace chrono =std::chrono;
using namespace std::chrono_literals;
namespace this_thread = std::this_thread;

using std::atomic;
using std::map;
using std::mutex;
using std::thread;

class spin_lock : public non_copyable_mixin<spin_lock> {
 public:
  struct no_backoff_strategy {
    bool operator()();
  };
  struct yield_backoff_strategy {
    bool operator()();
  };

 public:
  explicit spin_lock() = default;

  template <typename Backoff>
  void lock(Backoff backoff) noexcept {
    while (m_locked.test_and_set(std::memory_order_acquire)) {
      backoff();
    }
  }

  void lock() noexcept;
  void unlock() noexcept;

 protected:
  std::atomic_flag m_locked{false};
};

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

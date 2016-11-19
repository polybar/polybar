#pragma once

#include <mutex>

#include "common.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace threading_util {
  namespace locking_strategy {
    struct no_backoff {
      bool operator()() {
        return true;
      }
    };

    struct yield_backoff {
      bool operator()() {
        this_thread::yield();
        return false;
      }
    };
  }

  class spin_lock : public non_copyable_mixin<spin_lock> {
   public:
    /**
     * Construct spin_lock
     */
    spin_lock() = default;

    /**
     * Lock using custom strategy
     */
    template <typename Backoff>
    void lock(Backoff backoff) noexcept {
      while (m_locked.test_and_set(std::memory_order_acquire)) {
        backoff();
      }
    }

    /**
     * Lock using default strategy
     */
    void lock() noexcept {
      lock(locking_strategy::no_backoff{});
    }

    /**
     * Unlock
     */
    void unlock() noexcept {
      m_locked.clear(std::memory_order_release);
    }

   protected:
    std::atomic_flag m_locked{false};
  };
}

POLYBAR_NS_END

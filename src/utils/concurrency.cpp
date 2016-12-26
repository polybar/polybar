#include "utils/concurrency.hpp"

POLYBAR_NS

bool spin_lock::no_backoff_strategy::operator()() {
  return true;
}
bool spin_lock::yield_backoff_strategy::operator()() {
  this_thread::yield();
  return false;
}
void spin_lock::lock() noexcept {
  lock(no_backoff_strategy{});
}
void spin_lock::unlock() noexcept {
  m_locked.clear(std::memory_order_release);
}

namespace concurrency_util {
  size_t thread_id(const thread::id id) {
    static size_t idx{1_z};
    static mutex_wrapper<map<thread::id, size_t>> ids;
    std::lock_guard<decltype(ids)> lock(ids);
    if (ids.find(id) == ids.end()) {
      ids[id] = idx++;
    }
    return ids[id];
  }
}

POLYBAR_NS_END

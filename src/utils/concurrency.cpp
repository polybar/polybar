#include "utils/concurrency.hpp"

#include <map>

POLYBAR_NS

namespace concurrency_util {
  size_t thread_id(const thread::id id) {
    static size_t idx{1_z};
    static mutex_wrapper<std::map<thread::id, size_t>> ids;
    std::lock_guard<decltype(ids)> lock(ids);
    if (ids.find(id) == ids.end()) {
      ids[id] = idx++;
    }
    return ids[id];
  }
}  // namespace concurrency_util

POLYBAR_NS_END

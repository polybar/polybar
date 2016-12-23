#include "utils/concurrency.hpp"

POLYBAR_NS

namespace concurrency_util {
  size_t thread_id(const thread::id id) {
    static size_t idx{1_z};
    static mutex mtx;
    static map<thread::id, size_t> ids;
    std::lock_guard<mutex> lock(mtx);
    if (ids.find(id) == ids.end()) {
      ids[id] = idx++;
    }
    return ids[id];
  }
}

POLYBAR_NS_END

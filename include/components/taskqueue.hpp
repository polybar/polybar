#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace chrono = std::chrono;
using namespace std::chrono_literals;

class taskqueue : non_copyable_mixin<taskqueue> {
 public:
  struct deferred {
    using clock = chrono::high_resolution_clock;
    using duration = chrono::milliseconds;
    using timepoint = chrono::time_point<clock, duration>;
    using callback = function<void(size_t remaining)>;

    explicit deferred(string id, timepoint now, duration wait, callback fn, size_t count)
        : id(move(id)), func(move(fn)), now(move(now)), wait(move(wait)), count(move(count)) {}

    const string id;
    const callback func;
    timepoint now;
    duration wait;
    size_t count;
  };

 public:
  using make_type = unique_ptr<taskqueue>;
  static make_type make();

  explicit taskqueue();
  ~taskqueue();

  void defer(
      string id, deferred::duration ms, deferred::callback fn, deferred::duration offset = 0ms, size_t count = 1);
  void defer_unique(
      string id, deferred::duration ms, deferred::callback fn, deferred::duration offset = 0ms, size_t count = 1);

  bool exist(const string& id);
  bool purge(const string& id);

 protected:
  void tick();

 private:
  std::thread m_thread;
  std::mutex m_lock{};
  std::condition_variable m_hold;
  std::atomic_bool m_active{true};

  vector<unique_ptr<deferred>> m_deferred;
};

POLYBAR_NS_END

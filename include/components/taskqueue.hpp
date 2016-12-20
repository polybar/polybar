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
  struct deferred : non_copyable_mixin<deferred> {
    using timepoint = chrono::time_point<chrono::high_resolution_clock, chrono::milliseconds>;
    using callback = function<void()>;

    explicit deferred(string&& id, timepoint::time_point&& tp, callback&& fn)
        : id(forward<decltype(id)>(id)), when(forward<decltype(tp)>(tp)), func(forward<decltype(fn)>(fn)) {}

    const string id;
    const timepoint when;
    const callback func;
  };

 public:
  using make_type = unique_ptr<taskqueue>;
  static make_type make();

  explicit taskqueue();
  ~taskqueue();

  void defer(string&& id, deferred::timepoint::duration&& ms, deferred::callback&& fn);
  void defer_unique(string&& id, deferred::timepoint::duration&& ms, deferred::callback&& fn);

  bool has_deferred(string&& id);

 protected:
  void tick();

 private:
  std::thread m_thread;
  std::mutex m_lock;
  std::condition_variable m_hold;
  std::atomic_bool m_active{true};

  vector<unique_ptr<deferred>> m_deferred;
};

POLYBAR_NS_END

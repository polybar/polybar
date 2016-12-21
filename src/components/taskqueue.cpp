#include <algorithm>

#include "components/taskqueue.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

taskqueue::make_type taskqueue::make() {
  return factory_util::unique<taskqueue>();
}

taskqueue::taskqueue() {
  m_thread = std::thread([&] {
    while (m_active) {
      std::unique_lock<std::mutex> guard(m_lock);

      if (m_deferred.empty()) {
        m_hold.wait(guard);
      } else {
        auto now = deferred::clock::now();
        auto wait = m_deferred.front()->now + m_deferred.front()->wait;
        for (auto&& task : m_deferred) {
          auto when = task->now + task->wait;
          if (when < wait) {
            wait = move(when);
          }
        }
        if (wait > now) {
          m_hold.wait_for(guard, wait - now);
        }
      }
      if (!m_deferred.empty()) {
        guard.unlock();
        tick();
      }
    }
  });
}

taskqueue::~taskqueue() {
  if (m_active && m_thread.joinable()) {
    m_active = false;
    m_hold.notify_all();
    m_thread.join();
  }
}

void taskqueue::defer(
    string id, deferred::duration ms, deferred::callback fn, deferred::duration offset, size_t count) {
  std::unique_lock<std::mutex> guard(m_lock);
  deferred::timepoint now{chrono::time_point_cast<deferred::duration>(deferred::clock::now() + move(offset))};
  m_deferred.emplace_back(make_unique<deferred>(move(id), move(now), move(ms), move(fn), move(count)));
  guard.unlock();
  m_hold.notify_one();
}

void taskqueue::defer_unique(
    string id, deferred::duration ms, deferred::callback fn, deferred::duration offset, size_t count) {
  purge(id);
  std::unique_lock<std::mutex> guard(m_lock);
  deferred::timepoint now{chrono::time_point_cast<deferred::duration>(deferred::clock::now() + move(offset))};
  m_deferred.emplace_back(make_unique<deferred>(move(id), move(now), move(ms), move(fn), move(count)));
  guard.unlock();
  m_hold.notify_one();
}

void taskqueue::tick() {
  if (!m_lock.try_lock()) {
    return;
  }
  std::unique_lock<std::mutex> guard(m_lock, std::adopt_lock);
  auto now = chrono::time_point_cast<deferred::duration>(deferred::clock::now());
  vector<pair<deferred::callback, size_t>> cbs;
  for (auto it = m_deferred.rbegin(); it != m_deferred.rend(); ++it) {
    auto& task = *it;
    if (task->now + task->wait > now) {
      continue;
    } else if (task->count--) {
      cbs.emplace_back(make_pair(task->func, task->count));
      task->now = now;
    } else {
      m_deferred.erase(std::remove_if(m_deferred.begin(), m_deferred.end(),
                           [&](const unique_ptr<deferred>& d) { return d == task; }),
          m_deferred.end());
    }
  }
  guard.unlock();
  for (auto&& p : cbs) {
    p.first(p.second);
  }
}

bool taskqueue::purge(const string& id) {
  std::lock_guard<std::mutex> guard(m_lock);
  return m_deferred.erase(std::remove_if(m_deferred.begin(), m_deferred.end(),
                              [id](const unique_ptr<deferred>& d) { return d->id == id; }),
             m_deferred.end()) == m_deferred.end();
}

bool taskqueue::exist(const string& id) {
  std::lock_guard<std::mutex> guard(m_lock);
  for (const auto& task : m_deferred) {
    if (task->id == id) {
      return true;
    }
  }
  return false;
}

POLYBAR_NS_END

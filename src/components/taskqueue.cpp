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
        auto now = deferred::timepoint::clock::now();
        auto wait = m_deferred.front()->when;
        for (auto&& task : m_deferred) {
          if (task->when < wait) {
            wait = task->when;
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

void taskqueue::defer(string&& id, deferred::duration&& ms, deferred::callback&& fn) {
  std::unique_lock<std::mutex> guard(m_lock);
  auto when = chrono::time_point_cast<deferred::duration>(deferred::timepoint::clock::now() + ms);
  m_deferred.emplace_back(make_unique<deferred>(forward<decltype(id)>(id), move(when), forward<decltype(fn)>(fn)));
  guard.unlock();
  m_hold.notify_one();
}

void taskqueue::defer_unique(string&& id, deferred::duration&& ms, deferred::callback&& fn) {
  std::unique_lock<std::mutex> guard(m_lock);
  for (auto it = m_deferred.rbegin(); it != m_deferred.rend(); ++it) {
    if ((*it)->id == id) {
      m_deferred.erase(std::remove(m_deferred.begin(), m_deferred.end(), (*it)), m_deferred.end());
    }
  }
  auto when = chrono::time_point_cast<deferred::duration>(deferred::timepoint::clock::now() + ms);
  m_deferred.emplace_back(make_unique<deferred>(forward<decltype(id)>(id), move(when), forward<decltype(fn)>(fn)));
  guard.unlock();
  m_hold.notify_one();
}

void taskqueue::tick() {
  if (m_lock.try_lock()) {
    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);
    auto now = deferred::timepoint::clock::now();
    for (auto it = m_deferred.rbegin(); it != m_deferred.rend(); ++it) {
      if ((*it)->when <= now) {
        (*it)->func();
        m_deferred.erase(std::remove(m_deferred.begin(), m_deferred.end(), (*it)), m_deferred.end());
      }
    }
  }
}

bool taskqueue::has_deferred(string&& id) {
  std::lock_guard<std::mutex> guard(m_lock);
  for (const auto& task : m_deferred) {
    if (task->id == id) {
      return true;
    }
  }
  return false;
}

POLYBAR_NS_END

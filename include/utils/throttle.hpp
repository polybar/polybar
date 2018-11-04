#pragma once

#include <chrono>
#include <deque>

#include "common.hpp"
#include "components/logger.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace chrono = std::chrono;

namespace throttle_util {
  using timewindow = chrono::duration<double, std::milli>;
  using timepoint_clock = chrono::high_resolution_clock;
  using timepoint = timepoint_clock::time_point;
  using queue = std::deque<timepoint>;
  using limit = size_t;

  namespace strategy {
    struct try_once_or_leave_yolo {
      bool operator()(queue& q, limit l, timewindow);
    };
    struct wait_patiently_by_the_door {
      bool operator()(queue& q, limit l, timewindow);
    };
  }

  /**
   * Throttle events within a set window of time
   *
   * Example usage:
   * \code cpp
   *   auto t = throttle_util::make_throttler(2, 1s);
   *   if (t->passthrough())
   *     ...
   * \endcode
   */
  class event_throttler {
   public:
    /**
     * Construct throttler
     */
    explicit event_throttler(int limit, timewindow timewindow) : m_limit(limit), m_timewindow(timewindow) {}

    /**
     * Check if event is allowed to pass
     * using specified strategy
     */
    template <typename Strategy>
    bool passthrough(Strategy wait_strategy) {
      expire_timestamps();
      return wait_strategy(m_queue, m_limit, m_timewindow);
    }

    /**
     * Check if event is allowed to pass
     * using default strategy
     */
    bool passthrough() {
      return passthrough(strategy::try_once_or_leave_yolo{});
    }

   protected:
    /**
     * Expire old timestamps
     */
    void expire_timestamps() {
      auto now = timepoint_clock::now();
      while (m_queue.size() > 0) {
        if ((now - m_queue.front()) < m_timewindow)
          break;
        m_queue.pop_front();
      }
    }

   private:
    queue m_queue{};
    limit m_limit{};
    timewindow m_timewindow{};
  };

  using throttle_t = unique_ptr<event_throttler>;

  template <typename... Args>
  throttle_t make_throttler(Args&&... args) {
    return factory_util::unique<event_throttler>(forward<Args>(args)...);
  }
}

POLYBAR_NS_END

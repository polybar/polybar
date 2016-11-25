#include <thread>

#include "utils/throttle.hpp"

POLYBAR_NS

namespace throttle_util {
  namespace strategy {
    /**
     * Only pass events when there are slots available
     */
    bool try_once_or_leave_yolo::operator()(queue& q, limit l, timewindow /*unused*/) {
      if (q.size() >= l) {
        return false;
      }
      q.emplace_back(timepoint_clock::now());
      return true;
    }

    /**
     * If no slots are available, wait the required
     * amount of time for a slot to become available
     * then let the event pass
     */
    bool wait_patiently_by_the_door::operator()(queue& q, limit l, timewindow /*unused*/) {
      auto now = timepoint_clock::now();
      q.emplace_back(now);
      if (q.size() >= l) {
        std::this_thread::sleep_for(now - q.front());
      }
      return true;
    }
  }
}

POLYBAR_NS_END

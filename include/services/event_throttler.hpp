#pragma once

#include <chrono>
#include <deque>
#include <memory>
#include <string>

namespace event_throttler
{
  typedef std::chrono::duration<double, std::milli> timewindow_t;
  typedef std::chrono::high_resolution_clock timepoint_clock_t;
  typedef event_throttler::timepoint_clock_t::time_point timepoint_t;
  typedef std::deque<event_throttler::timepoint_t> queue_t;
  typedef std::size_t limit_t;
}

class EventThrottler
{
  event_throttler::queue_t event_queue;
  event_throttler::limit_t passthrough_limit;
  event_throttler::timewindow_t timewindow;

  protected:
    void expire();

  public:
    explicit EventThrottler(int limit, event_throttler::timewindow_t timewindow)
      : passthrough_limit(limit), timewindow(timewindow) {}

    bool passthrough();
};

#include <thread>

#include "services/event_throttler.hpp"
#include "services/logger.hpp"

using namespace event_throttler;

bool EventThrottler::passthrough()
{
  // Remove expired frames from the queue
  this->expire();

  // Place the new frame in the bottom of the deck
  this->event_queue.emplace_back(timepoint_clock_t::now());

  // Check if the limit has been reached
  if (this->event_queue.size() >= this->passthrough_limit) {
    // std::this_thread::sleep_for(this->timewindow * this->event_queue.size());
    std::this_thread::sleep_for(this->timewindow);

    if (this->event_queue.size() - 1 >= this->passthrough_limit) {
      log_trace("Event passthrough throttled");
      return false;
    }
  }

  return true;
}

void EventThrottler::expire()
{
  auto now = timepoint_clock_t::now();

  while (this->event_queue.size() > 0) {
    if ((now - this->event_queue.front()) < this->timewindow)
      break;

    this->event_queue.pop_front();
  }
}

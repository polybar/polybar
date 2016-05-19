#include <unistd.h>

#include "utils/timer.hpp"

namespace timer
{
  unsigned int seconds_to_microseconds(float seconds) {
    return seconds * 1000000;
  }

  void sleep(float seconds) {
    usleep(seconds_to_microseconds(seconds));
  }
}

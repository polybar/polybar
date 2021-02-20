#pragma once

#include <uv.h>

#include "common.hpp"

POLYBAR_NS

class eventloop {
 public:
  eventloop();
  ~eventloop();

  void run();

  void stop();

  /**
   * TODO remove
   */
  uv_loop_t* get() const {
    return m_loop.get();
  }

 private:
  std::unique_ptr<uv_loop_t> m_loop{nullptr};
};

POLYBAR_NS_END

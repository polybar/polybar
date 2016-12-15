#pragma once

#include <X11/Xutil.h>

#include "common.hpp"

POLYBAR_NS

namespace xlib {
  shared_ptr<Display> get_display();
  shared_ptr<Visual> get_visual(int screen = 0, uint8_t depth = 32);

  Colormap create_colormap(int screen = 0);

  /**
   * RAII wrapper for Xlib display locking
   */
  class display_lock {
   public:
    explicit display_lock(shared_ptr<Display>&& display);
    ~display_lock();

   protected:
    shared_ptr<Display> m_display;
  };

  inline auto make_display_lock();
}

POLYBAR_NS_END

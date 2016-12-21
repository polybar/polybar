#pragma once

#include <X11/Xutil.h>

#include "common.hpp"

POLYBAR_NS

namespace xlib {
  namespace detail {
    /**
     * RAII wrapper for Xlib display locking
     */
    class display_lock {
     public:
      explicit display_lock(Display* display);
      ~display_lock();

     protected:
      Display* m_display;
    };
  }

  Display* get_display();
  Visual* get_visual(int screen = 0, uint8_t depth = 32);
  Colormap create_colormap(int screen = 0);
  inline auto make_display_lock();
}

POLYBAR_NS_END

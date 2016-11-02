#pragma once

#include <X11/Xutil.h>

#include "common.hpp"

LEMONBUDDY_NS

namespace xlib {
  extern Display* g_display;
  extern XVisualInfo g_visual_info;

  extern Display* get_display();
  extern Visual* get_visual(int screen = 0);
  extern Colormap create_colormap(int screen = 0);

  /**
   * RAII wrapper for Xlib display locking
   */
  class xlib_lock {
   public:
    explicit xlib_lock(Display* display) {
      XLockDisplay(display);
      m_display = display;
    }

    ~xlib_lock() {
      XUnlockDisplay(m_display);
    }

   protected:
    Display* m_display;
  };

  inline auto make_display_lock() {
    return make_unique<xlib_lock>(get_display());
  }
}

LEMONBUDDY_NS_END

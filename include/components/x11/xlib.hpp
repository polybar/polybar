#pragma once

#include <X11/Xutil.h>

#include "common.hpp"
#include "components/x11/xutils.hpp"

LEMONBUDDY_NS

namespace xlib {
  static Display* g_display = nullptr;
  static Visual* g_visual = nullptr;

  /**
   * Get pointer of Xlib Display
   */
  inline Display* get_display() {
    if (g_display == nullptr)
      g_display = XOpenDisplay(nullptr);
    return g_display;
  }

  /**
   * Get pointer of Xlib visual
   */
  inline Visual* get_visual() {
    if (g_visual == nullptr) {
      XVisualInfo xv;
      xv.depth = 32;
      int result = 0;
      auto result_ptr = XGetVisualInfo(get_display(), VisualDepthMask, &xv, &result);

      if (result > 0)
        g_visual = result_ptr->visual;
      else
        g_visual = XDefaultVisual(get_display(), 0);

      free(result_ptr);
    }

    return g_visual;
  }

  /**
   * RAII wrapper for Xlib display locking
   */
  // class xlib_lock {
  //  public:
  //   explicit xlib_lock(Display* display) {
  //     XLockDisplay(display);
  //     m_display = display;
  //   }
  //
  //   ~xlib_lock() {
  //     XUnlockDisplay(m_display);
  //   }
  //
  //  protected:
  //   Display* m_display;
  // };

  // inline auto make_display_lock() {
  //   return make_unique<xlib_lock>(get_display());
  // }
}

LEMONBUDDY_NS_END

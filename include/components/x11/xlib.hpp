#pragma once

#include <X11/Xutil.h>

#include "common.hpp"

LEMONBUDDY_NS

namespace xlib {
  static Display* g_display = nullptr;
  static XVisualInfo g_visual_info;

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
  inline Visual* get_visual(int screen = 0) {
    if (g_visual_info.visual == nullptr) {
      XMatchVisualInfo(get_display(), screen, 32, TrueColor, &g_visual_info);
    }

    return g_visual_info.visual;
  }

  inline Colormap create_colormap(int screen = 0) {
    return XCreateColormap(get_display(), XRootWindow(get_display(), screen), get_visual(), screen);
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

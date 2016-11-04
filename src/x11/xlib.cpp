#include "x11/xlib.hpp"

LEMONBUDDY_NS

namespace xlib {
  Display* g_display = nullptr;
  XVisualInfo g_visual_info;

  /**
   * Get pointer of Xlib Display
   */
  Display* get_display() {
    if (g_display == nullptr)
      g_display = XOpenDisplay(nullptr);
    return g_display;
  }

  Visual* get_visual(int screen) {
    if (g_visual_info.visual == nullptr) {
      XMatchVisualInfo(get_display(), screen, 32, TrueColor, &g_visual_info);
    }

    return g_visual_info.visual;
  }

  Colormap create_colormap(int screen) {
    return XDefaultColormap(get_display(), screen);
  }
}

LEMONBUDDY_NS_END

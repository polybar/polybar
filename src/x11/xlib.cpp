#include "x11/xlib.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace xlib {
  shared_ptr<Display> g_display_ptr;
  shared_ptr<Visual> g_visual_ptr;

  shared_ptr<Display> get_display() {
    if (!g_display_ptr) {
      // g_display_ptr = shared_ptr<Display>(XOpenDisplay(nullptr), bind(XCloseDisplay, placeholders::_1));
      g_display_ptr = shared_ptr<Display>(XOpenDisplay(nullptr), factory_util::null_deleter{});
    }
    return g_display_ptr;
  }

  shared_ptr<Visual> get_visual(int screen) {
    if (!g_visual_ptr) {
      XVisualInfo info;
      if (XMatchVisualInfo(get_display().get(), screen, 32, TrueColor, &info)) {
        g_visual_ptr = shared_ptr<Visual>(info.visual, bind(XFree, placeholders::_1));
      }
    }

    return g_visual_ptr;
  }

  Colormap create_colormap(int screen) {
    return XDefaultColormap(get_display().get(), screen);
  }

  display_lock::display_lock(shared_ptr<Display>&& display) : m_display(forward<decltype(display)>(display)) {
    XLockDisplay(m_display.get());
  }

  display_lock::~display_lock() {
    XUnlockDisplay(m_display.get());
  }

  inline auto make_display_lock() {
    return make_unique<display_lock>(get_display());
  }
}

POLYBAR_NS_END

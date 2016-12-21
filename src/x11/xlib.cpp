#include <X11/X.h>

#include "x11/xlib.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace xlib {
  namespace detail {
    display_lock::display_lock(Display* display) : m_display(forward<decltype(display)>(display)) {
      XLockDisplay(m_display);
    }

    display_lock::~display_lock() {
      XUnlockDisplay(m_display);
    }
  }

  Display* get_display() {
    static Display* display{XOpenDisplay(nullptr)};
    return display;
  }

  Visual* get_visual(int screen, uint8_t depth) {
    static shared_ptr<Visual> visual;
    if (!visual) {
      XVisualInfo info{};
      if (XMatchVisualInfo(get_display(), screen, depth, TrueColor, &info)) {
        visual = shared_ptr<Visual>(info.visual, [=](Visual* v) { XFree(v); });
      }
    }
    return &*visual;
  }

  Colormap create_colormap(int screen) {
    return XDefaultColormap(get_display(), screen);
  }

  inline auto make_display_lock() {
    return make_unique<detail::display_lock>(get_display());
  }
}

POLYBAR_NS_END

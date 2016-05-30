#include <string>
#include <algorithm>
#include <cstring>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include "utils/memory.hpp"
#include "utils/xlib.hpp"
#include "services/logger.hpp"

namespace xlib
{
  std::vector<std::unique_ptr<Monitor>> get_sorted_monitorlist()
  {
    std::vector<std::unique_ptr<Monitor>> monitors;
    int n_monitors;

    Display *display = XOpenDisplay(nullptr);

    if (display == nullptr) {
      get_logger()->error("Could not open display");
      return monitors;
    }

    int screen = XDefaultScreen(display);
    Window root = XRootWindow(display, screen);
    XRRMonitorInfo *info = XRRGetMonitors(display, root, 1, &n_monitors);

    repeat(n_monitors)
    {
      char *name = XGetAtomName(display, info[repeat_i_rev(n_monitors)].name);

      auto monitor = std::make_unique<Monitor>();
      monitor->name = std::string(name);
      monitor->width = info[repeat_i_rev(n_monitors)].width;
      monitor->height = info[repeat_i_rev(n_monitors)].height;
      monitor->x = info[repeat_i_rev(n_monitors)].x;
      monitor->y = info[repeat_i_rev(n_monitors)].y;
      monitors.emplace_back(std::move(monitor));

      std::free(name);
    }

    std::free(info);

    XCloseDisplay(display);

    std::sort(monitors.begin(), monitors.end(), [](std::unique_ptr<Monitor> &m1, std::unique_ptr<Monitor> &m2) -> bool{
      if (m1->x < m2->x || m1->y + m1->height <= m2->y)
        return 1;
      if (m1->x > m2->x || m1->y + m1->height > m2->y)
        return -1;
      return 0;
    });

    int idx = 0;

    for (auto &mon : monitors) {
      mon->index = idx++;
    }

    return monitors;
  }

  std::unique_ptr<Monitor> get_monitor(const std::string& n_monitorsame)
  {
    auto monitor = std::make_unique<Monitor>();
    int n_monitors;

    Display *display = XOpenDisplay(nullptr);
    int screen = XDefaultScreen(display);
    Window root = XRootWindow(display, screen);
    XRRMonitorInfo *info = XRRGetMonitors(display, root, 1, &n_monitors);

    repeat(n_monitors)
    {
      char *name = XGetAtomName(display, info[repeat_i_rev(n_monitors)].name);

      if (std::strcmp(name, n_monitorsame.c_str()) != 0) {
        continue;
      }

      monitor->name = std::string(name);
      monitor->width = info[repeat_i_rev(n_monitors)].width;
      monitor->height = info[repeat_i_rev(n_monitors)].height;
      monitor->x = info[repeat_i_rev(n_monitors)].x;
      monitor->y = info[repeat_i_rev(n_monitors)].y;

      std::free(name);
    }

    std::free(info);

    XCloseDisplay(display);

    return monitor;
  }
}

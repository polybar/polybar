#include <xcb/xcb_icccm.h>

#include "x11/atoms.hpp"
#include "x11/wm.hpp"

LEMONBUDDY_NS

namespace wm_util {
  void set_wmname(connection& conn, xcb_window_t win, string wm_name, string wm_class) {
    xcb_icccm_set_wm_name(conn, win, XCB_ATOM_STRING, 8, wm_name.length(), wm_name.c_str());
    xcb_icccm_set_wm_class(conn, win, wm_class.length(), wm_class.c_str());
  }

  void set_wmprotocols(connection& conn, xcb_window_t win, vector<xcb_atom_t> flags) {
    xcb_icccm_set_wm_protocols(conn, win, WM_PROTOCOLS, flags.size(), flags.data());
  }

  void set_windowtype(connection& conn, xcb_window_t win, vector<xcb_atom_t> types) {
    conn.change_property(XCB_PROP_MODE_REPLACE, win, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 32,
        types.size(), types.data());
  }

  void set_wmstate(connection& conn, xcb_window_t win, vector<xcb_atom_t> states) {
    conn.change_property(
        XCB_PROP_MODE_REPLACE, win, _NET_WM_STATE, XCB_ATOM_ATOM, 32, states.size(), states.data());
  }

  void set_wmpid(connection& conn, xcb_window_t win, pid_t pid) {
    pid = getpid();
    conn.change_property(XCB_PROP_MODE_REPLACE, win, _NET_WM_PID, XCB_ATOM_CARDINAL, 32, 1, &pid);
  }

  void set_wmdesktop(connection& conn, xcb_window_t win, uint32_t desktop) {
    const uint32_t value_list[1]{desktop};
    conn.change_property(
        XCB_PROP_MODE_REPLACE, win, _NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1, value_list);
  }

  void set_trayorientation(connection& conn, xcb_window_t win, uint32_t orientation) {
    conn.change_property(XCB_PROP_MODE_REPLACE, win, _NET_SYSTEM_TRAY_ORIENTATION,
        _NET_SYSTEM_TRAY_ORIENTATION, 32, 1, &orientation);
  }

  void set_trayvisual(connection& conn, xcb_window_t win, xcb_visualid_t visual) {
    conn.change_property(
        XCB_PROP_MODE_REPLACE, win, _NET_SYSTEM_TRAY_VISUAL, XCB_ATOM_VISUALID, 32, 1, &visual);
  }
}

LEMONBUDDY_NS_END

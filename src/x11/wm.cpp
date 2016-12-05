#include <unistd.h>
#include <xcb/xcb_icccm.h>

#include "x11/atoms.hpp"
#include "x11/wm.hpp"

POLYBAR_NS

namespace wm_util {
  void set_wm_name(xcb_connection_t* conn, xcb_window_t win, const string& wm_name, const string& wm_class) {
    xcb_icccm_set_wm_name(conn, win, XCB_ATOM_STRING, 8, wm_name.length(), wm_name.c_str());
    xcb_icccm_set_wm_class(conn, win, wm_class.length(), wm_class.c_str());
  }

  void set_wm_protocols(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> flags) {
    xcb_icccm_set_wm_protocols(conn, win, WM_PROTOCOLS, flags.size(), flags.data());
  }

  void set_wm_window_type(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> types) {
    xcb_change_property(
        conn, XCB_PROP_MODE_REPLACE, win, _NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 32, types.size(), types.data());
  }

  void set_wm_state(xcb_connection_t* conn, xcb_window_t win, vector<xcb_atom_t> states) {
    xcb_change_property(
        conn, XCB_PROP_MODE_REPLACE, win, _NET_WM_STATE, XCB_ATOM_ATOM, 32, states.size(), states.data());
  }

  void set_wm_pid(xcb_connection_t* conn, xcb_window_t win, pid_t pid) {
    pid = getpid();
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, _NET_WM_PID, XCB_ATOM_CARDINAL, 32, 1, &pid);
  }

  void set_wm_desktop(xcb_connection_t* conn, xcb_window_t win, uint32_t desktop) {
    const uint32_t value_list[1]{desktop};
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win, _NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 32, 1, value_list);
  }
}

POLYBAR_NS_END

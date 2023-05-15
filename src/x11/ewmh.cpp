#include "x11/ewmh.hpp"

#include <unistd.h>

#include "components/types.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

namespace ewmh_util {

ewmh_connection::ewmh_connection() {
  xcb_ewmh_init_atoms_replies(&c, xcb_ewmh_init_atoms(connection::make(), &c), nullptr);
}

ewmh_connection::~ewmh_connection() {
  xcb_ewmh_connection_wipe(&c);
}

xcb_ewmh_connection_t* ewmh_connection::operator->() {
  return &c;
}

ewmh_connection::operator xcb_ewmh_connection_t*() {
  return &c;
}

ewmh_connection& initialize() {
  static ewmh_connection c;
  return c;
}

bool supports(xcb_atom_t atom, int screen) {
  auto& conn = initialize();
  xcb_ewmh_get_atoms_reply_t reply{};
  if (xcb_ewmh_get_supported_reply(conn, xcb_ewmh_get_supported(conn, screen), &reply, nullptr)) {
    for (size_t n = 0; n < reply.atoms_len; n++) {
      if (reply.atoms[n] == atom) {
        xcb_ewmh_get_atoms_reply_wipe(&reply);
        return true;
      }
    }
    xcb_ewmh_get_atoms_reply_wipe(&reply);
  }
  return false;
}

string get_wm_name(xcb_window_t win) {
  auto& conn = initialize();
  xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
  if (xcb_ewmh_get_wm_name_reply(conn, xcb_ewmh_get_wm_name(conn, win), &utf8_reply, nullptr)) {
    return get_reply_string(&utf8_reply);
  }
  return "";
}

string get_visible_name(xcb_window_t win) {
  auto& conn = initialize();
  xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
  if (xcb_ewmh_get_wm_visible_name_reply(conn, xcb_ewmh_get_wm_visible_name(conn, win), &utf8_reply, nullptr)) {
    return get_reply_string(&utf8_reply);
  }
  return "";
}

string get_icon_name(xcb_window_t win) {
  auto& conn = initialize();
  xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
  if (xcb_ewmh_get_wm_icon_name_reply(conn, xcb_ewmh_get_wm_icon_name(conn, win), &utf8_reply, nullptr)) {
    return get_reply_string(&utf8_reply);
  }
  return "";
}

string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply) {
  string str;
  if (reply) {
    str = string(reply->strings, reply->strings_len);
    xcb_ewmh_get_utf8_strings_reply_wipe(reply);
  }
  return str;
}

unsigned int get_current_desktop(int screen) {
  auto& conn = initialize();
  unsigned int desktop = XCB_NONE;
  xcb_ewmh_get_current_desktop_reply(conn, xcb_ewmh_get_current_desktop(conn, screen), &desktop, nullptr);
  return desktop;
}

unsigned int get_number_of_desktops(int screen) {
  auto& conn = initialize();
  unsigned int desktops = XCB_NONE;
  xcb_ewmh_get_number_of_desktops_reply(conn, xcb_ewmh_get_number_of_desktops(conn, screen), &desktops, nullptr);
  return desktops;
}

vector<position> get_desktop_viewports(int screen) {
  auto& conn = initialize();
  vector<position> viewports;
  xcb_ewmh_get_desktop_viewport_reply_t reply{};
  if (xcb_ewmh_get_desktop_viewport_reply(conn, xcb_ewmh_get_desktop_viewport(conn, screen), &reply, nullptr)) {
    for (size_t n = 0; n < reply.desktop_viewport_len; n++) {
      viewports.emplace_back(position{
          static_cast<short int>(reply.desktop_viewport[n].x), static_cast<short int>(reply.desktop_viewport[n].y)});
    }
  }
  return viewports;
}

vector<string> get_desktop_names(int screen) {
  auto& conn = initialize();
  xcb_ewmh_get_utf8_strings_reply_t reply{};
  if (xcb_ewmh_get_desktop_names_reply(conn, xcb_ewmh_get_desktop_names(conn, screen), &reply, nullptr)) {
    return string_util::split(string(reply.strings, reply.strings_len), '\0');
  }
  return {};
}

xcb_window_t get_active_window(int screen) {
  auto& conn = initialize();
  unsigned int win = XCB_NONE;
  xcb_ewmh_get_active_window_reply(conn, xcb_ewmh_get_active_window(conn, screen), &win, nullptr);
  return win;
}

void change_current_desktop(unsigned int desktop) {
  auto& conn = initialize();
  xcb_ewmh_request_change_current_desktop(conn, 0, desktop, XCB_CURRENT_TIME);
  xcb_flush(conn->connection);
}

unsigned int get_desktop_from_window(xcb_window_t window) {
  auto& conn = initialize();
  unsigned int desktop = XCB_NONE;
  xcb_ewmh_get_wm_desktop_reply(conn, xcb_ewmh_get_wm_desktop(conn, window), &desktop, nullptr);
  return desktop;
}

void set_wm_window_type(xcb_window_t win, vector<xcb_atom_t> types) {
  auto& conn = initialize();
  xcb_ewmh_set_wm_window_type(conn, win, types.size(), types.data());
  xcb_flush(conn->connection);
}

void set_wm_state(xcb_window_t win, vector<xcb_atom_t> states) {
  auto& conn = initialize();
  xcb_ewmh_set_wm_state(conn, win, states.size(), states.data());
  xcb_flush(conn->connection);
}

vector<xcb_atom_t> get_wm_state(xcb_window_t win) {
  auto& conn = initialize();
  xcb_ewmh_get_atoms_reply_t reply;
  if (xcb_ewmh_get_wm_state_reply(conn, xcb_ewmh_get_wm_state(conn, win), &reply, nullptr)) {
    return {reply.atoms, reply.atoms + reply.atoms_len};
  }
  return {};
}

void set_wm_pid(xcb_window_t win) {
  auto& conn = initialize();
  xcb_ewmh_set_wm_pid(conn, win, getpid());
  xcb_flush(conn->connection);
}

void set_wm_pid(xcb_window_t win, unsigned int pid) {
  auto& conn = initialize();
  xcb_ewmh_set_wm_pid(conn, win, pid);
  xcb_flush(conn->connection);
}

void set_wm_desktop(xcb_window_t win, unsigned int desktop) {
  auto& conn = initialize();
  xcb_ewmh_set_wm_desktop(conn, win, desktop);
  xcb_flush(conn->connection);
}

void set_wm_window_opacity(xcb_window_t win, unsigned long int values) {
  auto& conn = initialize();
  xcb_change_property(
      conn->connection, XCB_PROP_MODE_REPLACE, win, _NET_WM_WINDOW_OPACITY, XCB_ATOM_CARDINAL, 32, 1, &values);
  xcb_flush(conn->connection);
}

vector<xcb_window_t> get_client_list(int screen) {
  auto& conn = initialize();
  xcb_ewmh_get_windows_reply_t reply;
  if (xcb_ewmh_get_client_list_reply(conn, xcb_ewmh_get_client_list(conn, screen), &reply, nullptr)) {
    return {reply.windows, reply.windows + reply.windows_len};
  }
  return {};
}

/**
 * Retrieves the _NET_SUPPORTING_WM_CHECK atom on the given window
 *
 * @return The _NET_SUPPORTING_WM_CHECK window id or XCB_NONE in case of an error
 */
xcb_window_t get_supporting_wm_check(xcb_window_t win) {
  auto& conn = initialize();
  xcb_window_t result{};
  if (xcb_ewmh_get_supporting_wm_check_reply(conn, xcb_ewmh_get_supporting_wm_check(conn, win), &result, nullptr)) {
    return result;
  }
  return XCB_NONE;
}

/**
 * Searches for the meta window spawned by the WM to indicate a compliant WM is active.
 *
 * The window has the following properties:
 * * It's pointed to by the root window's _NET_SUPPORTING_WM_CHECK atom
 * * Its own _NET_SUPPORTING_WM_CHECK atom is set and points to itself
 *
 * @return The meta window id or XCB_NONE if it wasn't found
 */
xcb_window_t get_ewmh_meta_window(xcb_window_t root) {
  xcb_window_t wm_meta_window = ewmh_util::get_supporting_wm_check(root);

  if (wm_meta_window != XCB_NONE && ewmh_util::get_supporting_wm_check(wm_meta_window) == wm_meta_window) {
    return wm_meta_window;
  }

  return XCB_NONE;
}
} // namespace ewmh_util

POLYBAR_NS_END

#include <unistd.h>

#include "components/types.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"

POLYBAR_NS

namespace ewmh_util {
  ewmh_connection_t g_connection{nullptr};
  ewmh_connection_t initialize() {
    if (!g_connection) {
      g_connection = memory_util::make_malloc_ptr<xcb_ewmh_connection_t>(
          [=](xcb_ewmh_connection_t* c) { xcb_ewmh_connection_wipe(c); });
      xcb_ewmh_init_atoms_replies(&*g_connection, xcb_ewmh_init_atoms(connection::make(), &*g_connection), nullptr);
    }
    return g_connection;
  }

  bool supports(xcb_atom_t atom, int screen) {
    auto conn = initialize().get();
    bool supports{false};
    xcb_ewmh_get_atoms_reply_t reply{};
    reply.atoms = nullptr;
    if (xcb_ewmh_get_supported_reply(conn, xcb_ewmh_get_supported(conn, screen), &reply, nullptr)) {
      for (size_t n = 0; n < reply.atoms_len; ++n) {
        if (reply.atoms[n] == atom) {
          supports = true;
          break;
        }
      }
      if (reply.atoms != nullptr) {
        xcb_ewmh_get_atoms_reply_wipe(&reply);
      }
    }

    return supports;
  }

  string get_wm_name(xcb_window_t win) {
    auto conn = initialize().get();
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
    if (xcb_ewmh_get_wm_name_reply(conn, xcb_ewmh_get_wm_name(conn, win), &utf8_reply, nullptr)) {
      return get_reply_string(&utf8_reply);
    }
    return "";
  }

  string get_visible_name(xcb_window_t win) {
    auto conn = initialize().get();
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
    if (xcb_ewmh_get_wm_visible_name_reply(conn, xcb_ewmh_get_wm_visible_name(conn, win), &utf8_reply, nullptr)) {
      return get_reply_string(&utf8_reply);
    }
    return "";
  }

  string get_icon_name(xcb_window_t win) {
    auto conn = initialize().get();
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
    auto conn = initialize().get();
    unsigned int desktop = XCB_NONE;
    xcb_ewmh_get_current_desktop_reply(conn, xcb_ewmh_get_current_desktop(conn, screen), &desktop, nullptr);
    return desktop;
  }

  vector<position> get_desktop_viewports(int screen) {
    auto conn = initialize().get();
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
    auto conn = initialize().get();
    vector<string> names;
    xcb_ewmh_get_utf8_strings_reply_t reply{};
    if (xcb_ewmh_get_desktop_names_reply(conn, xcb_ewmh_get_desktop_names(conn, screen), &reply, nullptr)) {
      char buffer[BUFSIZ];
      size_t len{0};
      for (size_t n = 0; n < reply.strings_len; n++) {
        if (reply.strings[n] == '\0') {
          names.emplace_back(buffer, len);
          len = 0;
        } else {
          buffer[len++] = reply.strings[n];
        }
      }
      if (len) {
        names.emplace_back(buffer, len);
      }
    }
    return names;
  }

  xcb_window_t get_active_window(int screen) {
    auto conn = initialize().get();
    unsigned int win = XCB_NONE;
    xcb_ewmh_get_active_window_reply(conn, xcb_ewmh_get_active_window(conn, screen), &win, nullptr);
    return win;
  }

  void change_current_desktop(unsigned int desktop) {
    auto conn = initialize().get();
    xcb_ewmh_request_change_current_desktop(conn, 0, desktop, XCB_CURRENT_TIME);
    xcb_flush(conn->connection);
  }

  void set_wm_window_type(xcb_window_t win, vector<xcb_atom_t> types) {
    auto conn = initialize().get();
    xcb_ewmh_set_wm_window_type(conn, win, types.size(), types.data());
    xcb_flush(conn->connection);
  }

  void set_wm_state(xcb_window_t win, vector<xcb_atom_t> states) {
    auto conn = initialize().get();
    xcb_ewmh_set_wm_state(conn, win, states.size(), states.data());
    xcb_flush(conn->connection);
  }

  void set_wm_pid(xcb_window_t win) {
    auto conn = initialize().get();
    xcb_ewmh_set_wm_pid(conn, win, getpid());
    xcb_flush(conn->connection);
  }

  void set_wm_pid(xcb_window_t win, unsigned int pid) {
    auto conn = initialize().get();
    xcb_ewmh_set_wm_pid(conn, win, pid);
    xcb_flush(conn->connection);
  }

  void set_wm_desktop(xcb_window_t win, unsigned int desktop) {
    auto conn = initialize().get();
    xcb_ewmh_set_wm_desktop(conn, win, desktop);
    xcb_flush(conn->connection);
  }

  void set_wm_window_opacity(xcb_window_t win, unsigned long int values) {
    auto conn = initialize().get();
    xcb_change_property(conn->connection, XCB_PROP_MODE_REPLACE, win, _NET_WM_WINDOW_OPACITY, XCB_ATOM_CARDINAL, 32, 1, &values);
    xcb_flush(conn->connection);
  }
}

POLYBAR_NS_END

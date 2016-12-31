#include "x11/ewmh.hpp"
#include "components/types.hpp"
#include "x11/connection.hpp"

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

  bool supports(xcb_ewmh_connection_t* ewmh, xcb_atom_t atom, int screen) {
    bool supports{false};

    xcb_ewmh_get_atoms_reply_t reply{};
    reply.atoms = nullptr;

    if (!xcb_ewmh_get_supported_reply(ewmh, xcb_ewmh_get_supported(ewmh, screen), &reply, nullptr)) {
      return false;
    }

    for (size_t n = 0; n < reply.atoms_len; ++n) {
      if (reply.atoms[n] == atom) {
        supports = true;
        break;
      }
    }

    if (reply.atoms != nullptr) {
      xcb_ewmh_get_atoms_reply_wipe(&reply);
    }

    return supports;
  }

  string get_wm_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
    if (xcb_ewmh_get_wm_name_reply(conn, xcb_ewmh_get_wm_name(conn, win), &utf8_reply, nullptr)) {
      return get_reply_string(&utf8_reply);
    }
    return "";
  }

  string get_visible_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
    if (xcb_ewmh_get_wm_visible_name_reply(conn, xcb_ewmh_get_wm_visible_name(conn, win), &utf8_reply, nullptr)) {
      return get_reply_string(&utf8_reply);
    }
    return "";
  }

  string get_icon_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply{};
    if (xcb_ewmh_get_wm_icon_name_reply(conn, xcb_ewmh_get_wm_icon_name(conn, win), &utf8_reply, nullptr)) {
      return get_reply_string(&utf8_reply);
    }
    return "";
  }

  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply) {
    if (reply == nullptr) {
      return "";
    }
    string str(reply->strings, reply->strings_len);
    xcb_ewmh_get_utf8_strings_reply_wipe(reply);
    return str;
  }

  uint32_t get_current_desktop(xcb_ewmh_connection_t* conn, int screen) {
    uint32_t desktop{0};
    if (xcb_ewmh_get_current_desktop_reply(conn, xcb_ewmh_get_current_desktop(conn, screen), &desktop, nullptr)) {
      return desktop;
    }
    return XCB_NONE;
  }

  vector<position> get_desktop_viewports(xcb_ewmh_connection_t* conn, int screen) {
    vector<position> viewports;
    xcb_ewmh_get_desktop_viewport_reply_t reply{};

    if (!xcb_ewmh_get_desktop_viewport_reply(conn, xcb_ewmh_get_desktop_viewport(conn, screen), &reply, nullptr)) {
      return viewports;
    }

    for (size_t n = 0; n < reply.desktop_viewport_len; n++) {
      viewports.emplace_back(position{
          static_cast<int16_t>(reply.desktop_viewport[n].x), static_cast<int16_t>(reply.desktop_viewport[n].y)});
    }

    return viewports;
  }

  vector<string> get_desktop_names(xcb_ewmh_connection_t* conn, int screen) {
    vector<string> names;
    xcb_ewmh_get_utf8_strings_reply_t reply{};

    if (!xcb_ewmh_get_desktop_names_reply(conn, xcb_ewmh_get_desktop_names(conn, screen), &reply, nullptr)) {
      return names;
    }

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

    return names;
  }

  xcb_window_t get_active_window(xcb_ewmh_connection_t* conn, int screen) {
    xcb_window_t win{0};
    if (!xcb_ewmh_get_active_window_reply(conn, xcb_ewmh_get_active_window(conn, screen), &win, nullptr)) {
      return XCB_NONE;
    }
    return win;
  }

  void change_current_desktop(xcb_ewmh_connection_t* conn, uint32_t desktop) {
    xcb_ewmh_request_change_current_desktop(conn, 0, desktop, XCB_CURRENT_TIME);
  }
}

POLYBAR_NS_END

#include "x11/ewmh.hpp"
#include "x11/xutils.hpp"

POLYBAR_NS

namespace ewmh_util {
  ewmh_connection_t g_ewmh_connection{nullptr};

  ewmh_connection_t initialize() {
    if (!g_ewmh_connection) {
      g_ewmh_connection = memory_util::make_malloc_ptr<xcb_ewmh_connection_t>();
      auto* xconn = xutils::get_connection();
      auto* conn = g_ewmh_connection.get();
      xcb_ewmh_init_atoms_replies(conn, xcb_ewmh_init_atoms(xconn, conn), nullptr);
    }
    return g_ewmh_connection;
  }

  void dealloc() {
    if (g_ewmh_connection) {
      xcb_ewmh_connection_wipe(g_ewmh_connection.get());
      g_ewmh_connection.reset();
    }
  }

  bool supports(xcb_ewmh_connection_t* ewmh, xcb_atom_t atom, int screen) {
    bool supports{false};

    xcb_ewmh_get_atoms_reply_t reply;
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

  string get_visible_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply;
    if (!xcb_ewmh_get_wm_visible_name_reply(conn, xcb_ewmh_get_wm_visible_name(conn, win), &utf8_reply, nullptr)) {
      return "";
    }
    return get_reply_string(&utf8_reply);
  }

  string get_icon_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply;
    if (!xcb_ewmh_get_wm_icon_name_reply(conn, xcb_ewmh_get_wm_icon_name(conn, win), &utf8_reply, nullptr)) {
      return "";
    }
    return get_reply_string(&utf8_reply);
  }

  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply) {
    if (reply == nullptr || !reply->strings_len) {
      return "";
    }
    char buffer[BUFSIZ]{'\0'};
    strncpy(buffer, reply->strings, reply->strings_len);
    xcb_ewmh_get_utf8_strings_reply_wipe(reply);
    return buffer;
  }

  uint32_t get_current_desktop(xcb_ewmh_connection_t* conn, int screen) {
    uint32_t desktop{0};
    if (xcb_ewmh_get_current_desktop_reply(conn, xcb_ewmh_get_current_desktop(conn, screen), &desktop, nullptr)) {
      return desktop;
    }
    return XCB_NONE;
  }

  vector<string> get_desktop_names(xcb_ewmh_connection_t* conn, int screen) {
    xcb_ewmh_get_utf8_strings_reply_t reply;
    xcb_ewmh_get_desktop_names_reply(conn, xcb_ewmh_get_desktop_names(conn, screen), &reply, nullptr);

    vector<string> names;
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
    xcb_window_t win{XCB_NONE};
    xcb_ewmh_get_active_window_reply(conn, xcb_ewmh_get_active_window(conn, screen), &win, nullptr);
    return win;
  }
}

POLYBAR_NS_END

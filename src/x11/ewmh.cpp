#include "x11/ewmh.hpp"

LEMONBUDDY_NS

namespace ewmh_util {
  bool setup(connection& conn, xcb_ewmh_connection_t* dst) {
    return xcb_ewmh_init_atoms_replies(dst, xcb_ewmh_init_atoms(conn, dst), nullptr);
  }

  bool supports(xcb_ewmh_connection_t* ewmh, xcb_atom_t atom) {
    bool supports{false};

    xcb_ewmh_get_atoms_reply_t reply;
    reply.atoms = nullptr;

    if (!xcb_ewmh_get_supported_reply(ewmh, xcb_ewmh_get_supported(ewmh, 0), &reply, nullptr)) {
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

  xcb_window_t get_active_window(xcb_ewmh_connection_t* conn) {
    xcb_window_t win{XCB_NONE};
    xcb_ewmh_get_active_window_reply(conn, xcb_ewmh_get_active_window(conn, 0), &win, nullptr);
    return win;
  }

  string get_visible_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply;
    if (!xcb_ewmh_get_wm_visible_name_reply(conn, xcb_ewmh_get_wm_visible_name(conn, win), &utf8_reply, nullptr))
      return "";
    return get_reply_string(&utf8_reply);
  }

  string get_icon_name(xcb_ewmh_connection_t* conn, xcb_window_t win) {
    xcb_ewmh_get_utf8_strings_reply_t utf8_reply;
    if (!xcb_ewmh_get_wm_icon_name_reply(conn, xcb_ewmh_get_wm_icon_name(conn, win), &utf8_reply, nullptr))
      return "";
    return get_reply_string(&utf8_reply);
  }

  string get_reply_string(xcb_ewmh_get_utf8_strings_reply_t* reply) {
    if (reply == nullptr || !reply->strings_len)
      return "";
    char buffer[BUFSIZ]{'\0'};
    strncpy(buffer, reply->strings, reply->strings_len);
    xcb_ewmh_get_utf8_strings_reply_wipe(reply);
    return buffer;
  }
}

LEMONBUDDY_NS_END

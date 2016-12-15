#include "x11/icccm.hpp"

POLYBAR_NS

namespace icccm_util {
  string get_wm_name(xcb_connection_t* conn, xcb_window_t win) {
    xcb_icccm_get_text_property_reply_t reply{};
    if (xcb_icccm_get_wm_name_reply(conn, xcb_icccm_get_wm_name(conn, win), &reply, nullptr)) {
      return get_reply_string(&reply);
    }
    return "";
  }

  string get_reply_string(xcb_icccm_get_text_property_reply_t* reply) {
    if (reply->name == nullptr) {
      return "";
    }
    string str(reply->name, reply->name_len);
    xcb_icccm_get_text_property_reply_wipe(reply);
    return str;
  }
}

POLYBAR_NS_END

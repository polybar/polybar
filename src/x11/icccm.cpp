#include "x11/icccm.hpp"

#include "x11/atoms.hpp"

POLYBAR_NS

namespace icccm_util {
  string get_wm_name(xcb_connection_t* c, xcb_window_t w) {
    xcb_icccm_get_text_property_reply_t reply{};
    if (xcb_icccm_get_wm_name_reply(c, xcb_icccm_get_wm_name(c, w), &reply, nullptr)) {
      return get_reply_string(&reply);
    }
    return "";
  }

  pair<string, string> get_wm_class(xcb_connection_t* c, xcb_window_t w) {
    pair<string, string> result{"", ""};
    xcb_icccm_get_wm_class_reply_t reply{};
    if (xcb_icccm_get_wm_class_reply(c, xcb_icccm_get_wm_class(c, w), &reply, nullptr)) {
      result = {string(reply.instance_name), string(reply.class_name)};
      xcb_icccm_get_wm_class_reply_wipe(&reply);
    }
    return result;
  }

  string get_reply_string(xcb_icccm_get_text_property_reply_t* reply) {
    string str;
    if (reply) {
      str = string(reply->name, reply->name_len);
      xcb_icccm_get_text_property_reply_wipe(reply);
    }
    return str;
  }

  void set_wm_name(xcb_connection_t* c, xcb_window_t w, const char* wmname, size_t l, const char* wmclass, size_t l2) {
    xcb_icccm_set_wm_name(c, w, XCB_ATOM_STRING, 8, l, wmname);
    xcb_icccm_set_wm_class(c, w, l2, wmclass);
  }

  void set_wm_protocols(xcb_connection_t* c, xcb_window_t w, vector<xcb_atom_t> flags) {
    xcb_icccm_set_wm_protocols(c, w, WM_PROTOCOLS, flags.size(), flags.data());
  }

  bool get_wm_urgency(xcb_connection_t* c, xcb_window_t w) {
    xcb_icccm_wm_hints_t hints;
    if (xcb_icccm_get_wm_hints_reply(c, xcb_icccm_get_wm_hints(c, w), &hints, NULL)) {
      if (xcb_icccm_wm_hints_get_urgency(&hints) == XCB_ICCCM_WM_HINT_X_URGENCY)
        return true;
    }
    return false;
  }

  void set_wm_size_hints(xcb_connection_t* c, xcb_window_t w, int x, int y, int width, int height) {
    xcb_size_hints_t hints{};

    xcb_icccm_size_hints_set_size(&hints, false, width, height);
    xcb_icccm_size_hints_set_min_size(&hints, width, height);
    xcb_icccm_size_hints_set_max_size(&hints, width, height);
    xcb_icccm_size_hints_set_base_size(&hints, width, height);
    xcb_icccm_size_hints_set_position(&hints, false, x, y);

    xcb_icccm_set_wm_size_hints(c, w, XCB_ATOM_WM_NORMAL_HINTS, &hints);
  }
} // namespace icccm_util

POLYBAR_NS_END

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
      if(xcb_icccm_wm_hints_get_urgency(&hints) == XCB_ICCCM_WM_HINT_X_URGENCY)
        return true;
    }
    return false;
  }
}

POLYBAR_NS_END

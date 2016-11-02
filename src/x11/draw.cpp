#include <xcb/xcbext.h>

#include "utils/string.hpp"
#include "x11/color.hpp"
#include "x11/draw.hpp"

LEMONBUDDY_NS

namespace draw_util {
  /**
   * Fill region of drawable with color defined by gcontext
   */
  void fill(connection& c, xcb_drawable_t d, xcb_gcontext_t g, int16_t x, int16_t y, uint16_t w,
      uint16_t h) {
    array<xcb_rectangle_t, 1> rects{{xcb_rectangle_t({x, y, w, h})}};
    c.poly_fill_rectangle(d, g, rects.size(), rects.data());
  }

  /**
   * The xcb version of this function does not compose the correct request
   *
   * Code: http://wmdia.sourceforge.net/
   */
  xcb_void_cookie_t xcb_poly_text_16_patched(xcb_connection_t* conn, xcb_drawable_t d,
      xcb_gcontext_t gc, int16_t x, int16_t y, uint8_t len, uint16_t* str) {
    static const xcb_protocol_request_t xcb_req = {
        5,                 // count
        0,                 // ext
        XCB_POLY_TEXT_16,  // opcode
        1                  // isvoid
    };
    struct iovec xcb_parts[7];
    uint8_t xcb_lendelta[2];
    xcb_void_cookie_t xcb_ret;
    xcb_poly_text_8_request_t xcb_out;
    xcb_out.pad0 = 0;
    xcb_out.drawable = d;
    xcb_out.gc = gc;
    xcb_out.x = x;
    xcb_out.y = y;
    xcb_lendelta[0] = len;
    xcb_lendelta[1] = 0;
    xcb_parts[2].iov_base = reinterpret_cast<char*>(&xcb_out);
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    xcb_parts[4].iov_base = xcb_lendelta;
    xcb_parts[4].iov_len = sizeof(xcb_lendelta);
    xcb_parts[5].iov_base = reinterpret_cast<char*>(str);
    xcb_parts[5].iov_len = len * sizeof(int16_t);
    xcb_parts[6].iov_base = 0;
    xcb_parts[6].iov_len = -(xcb_parts[4].iov_len + xcb_parts[5].iov_len) & 3;
    xcb_ret.sequence = xcb_send_request(conn, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
  }
}

LEMONBUDDY_NS_END

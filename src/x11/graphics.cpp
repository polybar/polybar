#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_util.h>

#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/graphics.hpp"

POLYBAR_NS

namespace graphics_util {
  /**
   * Create a basic window
   */
  bool create_window(connection& conn, xcb_window_t* win, int16_t x, int16_t y, uint16_t w, uint16_t h, xcb_window_t root) {
    if (!root) {
      root = conn.screen()->root;
    }

    try {
      auto copy = XCB_COPY_FROM_PARENT;
      *win = conn.generate_id();
      conn.create_window_checked(copy, *win, root, x, y, w, h, 0, copy, copy, 0, nullptr);
      return true;
    } catch (const exception& err) {
      *win = XCB_NONE;
    }

    return false;
  }

  /**
   * Create a basic pixmap with the same depth as the
   * root depth of the default screen
   */
  bool create_pixmap(connection& conn, xcb_drawable_t dst, uint16_t w, uint16_t h, xcb_pixmap_t* pixmap) {
    return graphics_util::create_pixmap(conn, dst, w, h, conn.screen()->root_depth, pixmap);
  }

  /**
   * Create a basic pixmap with specific depth
   */
  bool create_pixmap(connection& conn, xcb_drawable_t dst, uint16_t w, uint16_t h, uint8_t d, xcb_pixmap_t* pixmap) {
    try {
      *pixmap = conn.generate_id();
      conn.create_pixmap_checked(d, *pixmap, dst, w, h);
      return true;
    } catch (const exception& err) {
      *pixmap = XCB_NONE;
    }

    return false;
  }

  /**
   * Create a basic gc
   */
  bool create_gc(connection& conn, xcb_drawable_t drawable, xcb_gcontext_t* gc) {
    try {
      xcb_params_gc_t params;

      uint32_t mask = 0;
      uint32_t values[32];

      XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 1);
      xutils::pack_values(mask, &params, values);

      *gc = conn.generate_id();
      conn.create_gc_checked(*gc, drawable, mask, values);
      return true;
    } catch (const exception& err) {
      *gc = XCB_NONE;
    }

    return false;
  }

  /**
   * Query for the root window pixmap
   */
  bool get_root_pixmap(connection& conn, root_pixmap* rpix) {
    auto screen = conn.screen();
    const xcb_atom_t pixmap_properties[3]{ESETROOT_PMAP_ID, _XROOTMAP_ID, _XSETROOT_ID};
    for (auto&& property : pixmap_properties) {
      auto cookie = xcb_get_property(conn, false, screen->root, property, XCB_ATOM_PIXMAP, 0L, 1L);
      auto reply = xcb_get_property_reply(conn, cookie, nullptr);

      if (reply && reply->format == 32 && reply->value_len == 1) {
        rpix->pixmap = *static_cast<xcb_pixmap_t*>(xcb_get_property_value(reply));
      }
    }

    if (!rpix->pixmap) {
      return false;
    }

    auto cookie = xcb_get_geometry(conn, rpix->pixmap);
    auto reply = xcb_get_geometry_reply(conn, cookie, nullptr);

    if (!reply) {
      rpix->pixmap = XCB_NONE;
      return false;
    }

    rpix->depth = reply->depth;
    rpix->width = reply->width;
    rpix->height = reply->height;
    rpix->x = reply->x;
    rpix->y = reply->y;

    return true;
  }
}

POLYBAR_NS_END

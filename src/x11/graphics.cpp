#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_util.h>

#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/graphics.hpp"

LEMONBUDDY_NS

namespace graphics_util {
  /**
   * Query for the root window pixmap
   */
  void get_root_pixmap(connection& conn, root_pixmap* rpix) {
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
      return;
    }

    auto cookie = xcb_get_geometry(conn, rpix->pixmap);
    auto reply = xcb_get_geometry_reply(conn, cookie, nullptr);

    if (!reply) {
      rpix->pixmap = 0;
      return;
    }

    rpix->depth = reply->depth;
    rpix->width = reply->width;
    rpix->height = reply->height;
    rpix->x = reply->x;
    rpix->y = reply->y;
  }

  /**
   * Create a basic gc
   */
  void simple_gc(connection& conn, xcb_drawable_t drawable, xcb_gcontext_t* gc) {
    xcb_params_gc_t params;

    uint32_t mask = 0;
    uint32_t values[32];

    XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, false);
    xutils::pack_values(mask, &params, values);

    try {
      *gc = conn.generate_id();
      conn.create_gc_checked(*gc, drawable, mask, values);
    } catch (const std::exception& err) {
      // no-op
    }
  }

  /**
   * Create a basic pixmap
   */
  void simple_pixmap(connection& conn, xcb_window_t dst, int w, int h, xcb_pixmap_t* pixmap) {
    try {
      *pixmap = conn.generate_id();
      conn.create_pixmap_checked(conn.screen()->root_depth, *pixmap, dst, w, h);
    } catch (const std::exception& err) {
      // no-op
    }
  }
}

LEMONBUDDY_NS_END

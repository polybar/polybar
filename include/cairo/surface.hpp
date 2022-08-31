#pragma once

#include <cairo/cairo-xcb.h>

#include "cairo/types.hpp"
#include "common.hpp"
#include "errors.hpp"
#include "utils/color.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace cairo {
  /**
   * @brief Base surface
   */
  class surface {
   public:
    explicit surface(cairo_surface_t* s) : m_s(s) {}
    virtual ~surface() {
      cairo_surface_destroy(m_s);
    }

    operator cairo_surface_t*() const {
      return m_s;
    }

    void flush() {
      cairo_surface_flush(m_s);
    }

    void show(bool clear = true) {
      if (clear) {
        cairo_surface_show_page(m_s);
      } else {
        cairo_surface_copy_page(m_s);
      }
    }

    void dirty() {
      cairo_surface_mark_dirty(m_s);
    }

    void dirty(const rect& r) {
      cairo_surface_mark_dirty_rectangle(m_s, r.x, r.y, r.w, r.h);
    }

    void write_png(const string& dst) {
      auto status = cairo_surface_write_to_png(m_s, dst.c_str());
      if (status != CAIRO_STATUS_SUCCESS) {
        throw application_error(sstream() << "cairo_surface_write_to_png(): " << cairo_status_to_string(status));
      }
    }

   protected:
    cairo_surface_t* m_s;
  };

  /**
   * @brief Surface for xcb
   */
  class xcb_surface : public surface {
   public:
    explicit xcb_surface(xcb_connection_t* c, xcb_pixmap_t p, xcb_visualtype_t* v, int w, int h)
        : surface(cairo_xcb_surface_create(c, p, v, w, h)) {}

    ~xcb_surface() override {}

    void set_drawable(xcb_drawable_t d, int w, int h) {
      cairo_surface_flush(m_s);
      cairo_xcb_surface_set_drawable(m_s, d, w, h);
    }
  };
}

POLYBAR_NS_END

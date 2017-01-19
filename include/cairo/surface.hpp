#pragma once

#include <cairo/cairo-xcb.h>

#include "cairo/types.hpp"
#include "common.hpp"
#include "utils/color.hpp"

POLYBAR_NS

namespace cairo {
  class surface {
   public:
    explicit surface(cairo_surface_t* s) : m_s(s) {}
    virtual ~surface() {
      cairo_surface_destroy(m_s);
    }

    operator cairo_surface_t*() const {
      return m_s;
    }

    surface& flush() {
      cairo_surface_flush(m_s);
      return *this;
    }

    surface& dirty() {
      cairo_surface_mark_dirty(m_s);
      return *this;
    }

    surface& dirty(const rect& r) {
      cairo_surface_mark_dirty_rectangle(m_s, r.x, r.y, r.w, r.h);
      return *this;
    }

   protected:
    cairo_surface_t* m_s;
  };

  class xcb_surface : public surface {
   public:
    explicit xcb_surface(xcb_connection_t* c, xcb_pixmap_t p, xcb_visualtype_t* v, int w, int h)
        : surface(cairo_xcb_surface_create(c, p, v, w, h)) {}
    ~xcb_surface() override {}
  };
}

POLYBAR_NS_END

#include "cairo/surface.hpp"
#include "cairo/context.hpp"
#include "events/signal.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/background_manager.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"

POLYBAR_NS

background_manager& background_manager::make() {
  return *factory_util::singleton<background_manager>(connection::make(), signal_emitter::make(), logger::make());
}

background_manager::background_manager(
    connection& conn, signal_emitter& sig, const logger& log)
  : m_connection(conn)
  , m_sig(sig)
  , m_log(log) {
  m_sig.attach(this);
}

background_manager::~background_manager() {
  m_sig.detach(this);
  free_resources();
}

std::shared_ptr<bg_slice> background_manager::observe(xcb_rectangle_t rect, xcb_window_t window) {
  // allocate a slice
  activate();
  auto slice = std::shared_ptr<bg_slice>(new bg_slice(m_connection, m_log, rect, window, m_visual));

  // make sure that we receive a notification when the background changes
  if(!m_attached) {
    m_connection.ensure_event_mask(m_connection.root(), XCB_EVENT_MASK_PROPERTY_CHANGE);
    m_connection.flush();
    m_connection.attach_sink(this, SINK_PRIORITY_SCREEN);
    m_attached = true;
  }

  // if the slice is empty, don't add to slices
  if (slice->m_rect.width == 0 || slice->m_rect.height == 0) {
    return slice;
  }

  m_slices.push_back(slice);
  fetch_root_pixmap();
  return slice;
}

void background_manager::deactivate() {
  if(m_attached) {
    m_connection.detach_sink(this, SINK_PRIORITY_SCREEN);
    m_attached = false;
  }
  free_resources();
}


void background_manager::activate() {
  if(!m_visual) {
    m_log.trace("background_manager: Finding root visual");
    m_visual = m_connection.visual_type_for_id(m_connection.screen(), m_connection.screen()->root_visual);
    m_log.trace("background_manager: Got root visual with depth %d", m_connection.screen()->root_depth);
  }
}

void background_manager::free_resources() {
  m_visual = nullptr;
}

void background_manager::fetch_root_pixmap() {
  m_log.trace("background_manager: Fetching pixmap");

  int pixmap_depth;
  xcb_pixmap_t pixmap;
  xcb_rectangle_t pixmap_geom;

  try {
    if (!m_connection.root_pixmap(&pixmap, &pixmap_depth, &pixmap_geom)) {
      return m_log.warn("background_manager: Failed to get root pixmap, default to black (is there a wallpaper?)");
    };
    m_log.trace("background_manager: root pixmap (%d:%d) %dx%d+%d+%d", pixmap, pixmap_depth,
                pixmap_geom.width, pixmap_geom.height, pixmap_geom.x, pixmap_geom.y);

    if (pixmap_depth == 1 && pixmap_geom.width == 1 && pixmap_geom.height == 1) {
      return m_log.err("background_manager: Cannot find root pixmap, try a different tool to set the desktop background");
    }

    for (auto it = m_slices.begin(); it != m_slices.end(); ) {
      auto slice = it->lock();
      if (!slice) {
        it = m_slices.erase(it);
        continue;
      }

      // fill the slice
      auto translated = m_connection.translate_coordinates(slice->m_window, m_connection.screen()->root, slice->m_rect.x, slice->m_rect.y);
      auto src_x = math_util::cap(translated->dst_x, pixmap_geom.x, int16_t(pixmap_geom.x + pixmap_geom.width));
      auto src_y = math_util::cap(translated->dst_y, pixmap_geom.y, int16_t(pixmap_geom.y + pixmap_geom.height));
      auto w = math_util::cap(slice->m_rect.width, uint16_t(0), uint16_t(pixmap_geom.width - (src_x - pixmap_geom.x)));
      auto h = math_util::cap(slice->m_rect.height, uint16_t(0), uint16_t(pixmap_geom.height - (src_y - pixmap_geom.y)));
      m_log.trace("background_manager: Copying from root pixmap (%d:%d) %dx%d+%d+%d", pixmap, pixmap_depth, w, h, src_x, src_y);
      m_connection.copy_area_checked(pixmap, slice->m_pixmap, slice->m_gcontext, src_x, src_y, 0, 0, w, h);

      it++;
    }

    // if there are no active slices, deactivate
    if (m_slices.empty()) {
      m_log.trace("background_manager: deactivating because there are no slices to observe");
      deactivate();
    }

  } catch(const exception& err) {
    m_log.err("background_manager: Failed to copy slice of root pixmap (%s)", err.what());
    throw;
  }

}

void background_manager::handle(const evt::property_notify& evt) {
  // if there are no slices to observe, don't do anything
  if(m_slices.empty()) {
    return;
  }

  if (evt->atom == _XROOTPMAP_ID || evt->atom == _XSETROOT_ID || evt->atom == ESETROOT_PMAP_ID) {
    fetch_root_pixmap();
    m_sig.emit(signals::ui::update_background());
  }
}

bool background_manager::on(const signals::ui::update_geometry&) {
  // if there are no slices to observe, don't do anything
  if(m_slices.empty()) {
    return false;
  }

  fetch_root_pixmap();
  m_sig.emit(signals::ui::update_background());
  return false;
}


bg_slice::bg_slice(connection& conn, const logger& log, xcb_rectangle_t rect, xcb_window_t window, xcb_visualtype_t* visual)
  : m_connection(conn)
  , m_rect(rect)
  , m_window(window) {
  try {
    allocate_resources(log, visual);
  } catch(...) {
    free_resources();
    throw;
  }
}

bg_slice::~bg_slice() {
  free_resources();
}

void bg_slice::allocate_resources(const logger& log, xcb_visualtype_t* visual) {
  if(m_pixmap == XCB_NONE) {
    log.trace("background_manager: Allocating pixmap");
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap(m_connection.screen()->root_depth, m_pixmap, m_window, m_rect.width, m_rect.height);
  }

  if(m_gcontext == XCB_NONE) {
    log.trace("background_manager: Allocating graphics context");
    auto black_pixel = m_connection.screen()->black_pixel;
    unsigned int mask = XCB_GC_GRAPHICS_EXPOSURES | XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
    unsigned int value_list[3] = {black_pixel, black_pixel, 0};
    m_gcontext = m_connection.generate_id();
    m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list);
  }

  if(!m_surface) {
    log.trace("background_manager: Allocating cairo surface");
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, visual, m_rect.width, m_rect.height);
  }

  // fill (to provide a default in the case that fetching the background fails)
  xcb_rectangle_t rect{0, 0, m_rect.width, m_rect.height};
  m_connection.poly_fill_rectangle(m_pixmap, m_gcontext, 1, &rect);
}

void bg_slice::free_resources() {
  m_surface.release();

  if(m_pixmap != XCB_NONE) {
    m_connection.free_pixmap(m_pixmap);
    m_pixmap = XCB_NONE;
  }

  if(m_gcontext != XCB_NONE) {
    m_connection.free_gc(m_gcontext);
    m_gcontext = XCB_NONE;
  }
}

POLYBAR_NS_END

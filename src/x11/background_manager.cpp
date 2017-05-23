#include "cairo/surface.hpp"
#include "events/signal.hpp"
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
}

background_manager::~background_manager() {
  free_resources();
}

cairo::surface* background_manager::get_surface() const {
  return m_surface.get();
}

void background_manager::activate(xcb_window_t window, xcb_rectangle_t rect) {
  // ensure that we start from a clean state
  //
  // the size of the pixmap may need to be changed, etc.
  // so the easiest way is to just re-allocate everything.
  // it may be possible to be more clever here, but activate is
  // not supposed to be called often so this shouldn't be a problem.
  free_resources();

  // make sure that we receive a notification when the background changes
  if(!m_attached) {
    m_connection.ensure_event_mask(m_connection.root(), XCB_EVENT_MASK_PROPERTY_CHANGE);
    m_connection.flush();
    m_connection.attach_sink(this, SINK_PRIORITY_SCREEN);
  }

  m_window = window;
  m_rect = rect;
  fetch_root_pixmap();
}

void background_manager::deactivate() {
  free_resources();
}


void background_manager::allocate_resources() {
  if(!m_visual) {
    m_log.trace("background_manager: Finding root visual");
    m_visual = m_connection.visual_type_for_id(m_connection.screen(), m_connection.screen()->root_visual);
    m_log.trace("background_manager: Got root visual with depth %d", m_connection.screen()->root_depth);
  }

  if(m_pixmap == XCB_NONE) {
    m_log.trace("background_manager: Allocating pixmap");
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap(m_connection.screen()->root_depth, m_pixmap, m_window, m_rect.width, m_rect.height);
  }

  if(m_gcontext == XCB_NONE) {
    m_log.trace("background_manager: Allocating graphics context");
    unsigned int mask = XCB_GC_GRAPHICS_EXPOSURES;
    unsigned int value_list[1] = {0};
    m_gcontext = m_connection.generate_id();
    m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list);
  }

  if(!m_surface) {
    m_log.trace("background_manager: Allocating cairo surface");
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, m_visual, m_rect.width, m_rect.height);
  }

  if(m_attached) {
    m_connection.detach_sink(this, SINK_PRIORITY_SCREEN);
    m_attached = false;
  }

}

void background_manager::free_resources() {
  m_surface.release();
  m_visual = nullptr;

  if(m_pixmap != XCB_NONE) {
    m_connection.free_pixmap(m_pixmap);
    m_pixmap = XCB_NONE;
  }

  if(m_gcontext != XCB_NONE) {
    m_connection.free_gc(m_gcontext);
    m_gcontext = XCB_NONE;
  }
}

void background_manager::fetch_root_pixmap() {
  allocate_resources();
  m_log.trace("background_manager: Fetching pixmap");

  int pixmap_depth;
  xcb_pixmap_t pixmap;
  xcb_rectangle_t pixmap_geom;

  if (!m_connection.root_pixmap(&pixmap, &pixmap_depth, &pixmap_geom)) {
    free_resources();
    return m_log.err("background_manager: Failed to get root pixmap for background (realloc=%i)", realloc);
  };

  auto src_x = math_util::cap(m_rect.x, pixmap_geom.x, int16_t(pixmap_geom.x + pixmap_geom.width));
  auto src_y = math_util::cap(m_rect.y, pixmap_geom.y, int16_t(pixmap_geom.y + pixmap_geom.height));
  auto h = math_util::min(m_rect.height, pixmap_geom.height);
  auto w = math_util::min(m_rect.width, pixmap_geom.width);

  m_log.trace("background_manager: Copying from root pixmap (%d) %dx%d+%dx%d", pixmap, w, h, src_x, src_y);
  try {
    m_connection.copy_area_checked(pixmap, m_pixmap, m_gcontext, src_x, src_y, 0, 0, w, h);
  } catch (const exception& err) {
    m_log.err("background_manager: Failed to copy slice of root pixmap (%s)", err.what());
    free_resources();
    return;
  }
}

void background_manager::handle(const evt::property_notify& evt) {
  // if region that we should observe is empty, don't do anything
  if(m_rect.width == 0 || m_rect.height == 0) return;

  if (evt->atom == _XROOTMAP_ID || evt->atom == _XSETROOT_ID || evt->atom == ESETROOT_PMAP_ID) {
    fetch_root_pixmap();
    m_sig.emit(signals::ui::update_background());
  }
}

POLYBAR_NS_END

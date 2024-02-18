#include "x11/background_manager.hpp"

#include <cassert>

#include "cairo/context.hpp"
#include "cairo/surface.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "events/signal.hpp"
#include "utils/factory.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

background_manager& background_manager::make() {
  return *factory_util::singleton<background_manager>(connection::make(), signal_emitter::make(), logger::make());
}

background_manager::background_manager(connection& conn, signal_emitter& sig, const logger& log)
    : m_connection(conn), m_sig(sig), m_log(log) {
  m_sig.attach(this);
}

background_manager::~background_manager() {
  m_sig.detach(this);
  free_resources();
}

std::shared_ptr<bg_slice> background_manager::observe(xcb_rectangle_t rect, xcb_window_t window) {
  // allocate a slice
  auto slice = std::shared_ptr<bg_slice>(new bg_slice(m_connection, m_log, rect, window));

  // if the slice is empty, don't add to slices
  if (slice->m_rect.width == 0 || slice->m_rect.height == 0) {
    return slice;
  }

  activate();

  m_slices.push_back(slice);

  update_slice(*slice);
  return slice;
}

void background_manager::activate() {
  attach();
}

void background_manager::deactivate() {
  detach();
  free_resources();
}

/**
 * Attaches a listener to listen for background changes on the root window.
 */
void background_manager::attach() {
  if (!m_attached) {
    // make sure that we receive a notification when the background changes
    m_connection.ensure_event_mask(m_connection.root(), XCB_EVENT_MASK_PROPERTY_CHANGE);
    m_connection.flush();
    m_connection.attach_sink(this, SINK_PRIORITY_SCREEN);
    m_attached = true;
  }
}

/**
 * Stops listening for background changes
 */
void background_manager::detach() {
  if (m_attached) {
    m_connection.detach_sink(this, SINK_PRIORITY_SCREEN);
    m_attached = false;
  }
}

void background_manager::free_resources() {
  clear_pixmap();
}

/**
 * Changes required when the background may have changed.
 *
 * 1. Delete cached pixmaps
 * 2. Update all slices (loads new pixmaps on demand)
 * 3. Notify about new background
 */
void background_manager::on_background_change() {
  clear_pixmap();
  m_pixmap_load_failed = false;

  for (auto it = m_slices.begin(); it != m_slices.end();) {
    auto slice = it->lock();
    if (!slice) {
      it = m_slices.erase(it);
      continue;
    }

    update_slice(*slice);
    it++;
  }

  // if there are no active slices, deactivate
  if (m_slices.empty()) {
    m_log.trace("background_manager: deactivating because there are no slices to observe");
    deactivate();
    return;
  }

  m_sig.emit(signals::ui::update_background());
}

void background_manager::update_slice(bg_slice& slice) {
  ensure_pixmap();

  if (has_pixmap()) {
    slice.copy(m_pixmap, m_pixmap_depth, m_pixmap_geom, m_visual);
  } else {
    slice.clear();
  }
}

void background_manager::handle(const evt::property_notify& evt) {
  if (evt->atom == _XROOTPMAP_ID || evt->atom == _XSETROOT_ID || evt->atom == ESETROOT_PMAP_ID) {
    m_log.trace("background_manager: root pixmap change");
    on_background_change();
  }
}

bool background_manager::on(const signals::ui::update_geometry&) {
  m_log.trace("background_manager: update_geometry");
  on_background_change();
  return false;
}

bool background_manager::has_pixmap() const {
  return m_pixmap != XCB_NONE;
}

void background_manager::ensure_pixmap() {
  // Only try to load the root pixmap if we haven't already loaded it and the previous load didn't fail.
  if (!has_pixmap() && !m_pixmap_load_failed) {
    load_pixmap();
    m_pixmap_load_failed = !has_pixmap();
  }
}

void background_manager::load_pixmap() {
  int old_depth = m_pixmap_depth;
  clear_pixmap();

  try {
    if (!m_connection.root_pixmap(&m_pixmap, &m_pixmap_depth, &m_pixmap_geom)) {
      m_log.warn("background_manager: Failed to get root pixmap, default to black (is there a wallpaper?)");
      return;
    }
  } catch (const exception& err) {
    m_log.err("background_manager: Failed to get root pixmap, default to black (%s)", err.what());
    clear_pixmap();
    return;
  }

  m_log.trace("background_manager: root pixmap (0x%x:%d) %dx%d+%d+%d", m_pixmap, m_pixmap_depth, m_pixmap_geom.width,
      m_pixmap_geom.height, m_pixmap_geom.x, m_pixmap_geom.y);

  if (m_pixmap_depth == 1 && m_pixmap_geom.width == 1 && m_pixmap_geom.height == 1) {
    m_log.err("background_manager: Cannot find root pixmap, try a different tool to set the desktop background");
    clear_pixmap();
    return;
  }

  // Only reload visual if depth changed
  if (old_depth != m_pixmap_depth) {
    m_visual = m_connection.visual_type(XCB_VISUAL_CLASS_TRUE_COLOR, m_pixmap_depth);

    if (!m_visual) {
      m_log.err("background_manager: Cannot find true color visual for root pixmap (depth: %d)", m_pixmap_depth);
      clear_pixmap();
      return;
    }
  }
}

void background_manager::clear_pixmap() {
  if (has_pixmap()) {
    m_pixmap = XCB_NONE;
    m_pixmap_depth = 0;
    m_pixmap_geom = {0, 0, 0, 0};
    m_visual = nullptr;
  }
}

bg_slice::bg_slice(connection& conn, const logger& log, xcb_rectangle_t rect, xcb_window_t window)
    : m_connection(conn), m_log(log), m_rect(rect), m_window(window) {}

bg_slice::~bg_slice() {
  free_resources();
}

/**
 * Get the current desktop background at the location of this slice.
 * The returned pointer is only valid as long as the slice itself is alive.
 *
 * This function is fast, since the current desktop background is cached.
 */
cairo::surface* bg_slice::get_surface() const {
  return m_surface.get();
}

void bg_slice::clear() {
  ensure_resources(0, nullptr);
}

void bg_slice::copy(xcb_pixmap_t root_pixmap, int depth, xcb_rectangle_t geom, xcb_visualtype_t* visual) {
  assert(root_pixmap);
  assert(depth > 0);
  ensure_resources(depth, visual);
  assert(m_pixmap);

  auto pixmap_end_x = int16_t(geom.x + geom.width);
  auto pixmap_end_y = int16_t(geom.y + geom.height);

  auto translated = m_connection.translate_coordinates(m_window, m_connection.screen()->root, m_rect.x, m_rect.y);

  /*
   * If the slice is not fully contained in the root pixmap, we will be missing at least some background pixels. For
   * those areas, nothing is copied over and a simple black background is shown.
   * This can happen when connecting new monitors without updating the root pixmap.
   */
  if (!(translated->dst_x >= geom.x && translated->dst_x + m_rect.width <= pixmap_end_x &&
          translated->dst_y >= geom.y && translated->dst_y + m_rect.height <= pixmap_end_y)) {
    m_log.err(
        "background_manager: Root pixmap does not fully cover transparent areas. "
        "Pseudo-transparency may not fully work and instead just show a black background. "
        "Make sure you have a wallpaper set on all of your screens");
  }

  /*
   * Coordinates of the slice in the root pixmap. The rectangle is capped so that it is contained in the root pixmap to
   * avoid copying areas not covered by the pixmap.
   */
  auto src_x = math_util::cap(translated->dst_x, geom.x, pixmap_end_x);
  auto src_y = math_util::cap(translated->dst_y, geom.y, pixmap_end_x);
  auto w = math_util::cap(m_rect.width, uint16_t(0), uint16_t(geom.width - (src_x - geom.x)));
  auto h = math_util::cap(m_rect.height, uint16_t(0), uint16_t(geom.height - (src_y - geom.y)));

  // fill the slice
  m_log.trace(
      "background_manager: Copying from root pixmap (0x%x:%d) %dx%d+%d+%d", root_pixmap, depth, w, h, src_x, src_y);
  m_connection.copy_area_checked(root_pixmap, m_pixmap, m_gcontext, src_x, src_y, 0, 0, w, h);
}

void bg_slice::ensure_resources(int depth, xcb_visualtype_t* visual) {
  if (m_depth != depth) {
    m_depth = depth;

    free_resources();

    if (depth != 0) {
      allocate_resources(visual);
    }
  }
}

void bg_slice::allocate_resources(xcb_visualtype_t* visual) {
  m_log.trace("background_manager: Allocating pixmap");
  m_pixmap = m_connection.generate_id();
  m_connection.create_pixmap(m_depth, m_pixmap, m_window, m_rect.width, m_rect.height);

  m_log.trace("background_manager: Allocating graphics context");
  auto black_pixel = m_connection.screen()->black_pixel;
  uint32_t mask = 0;
  xcb_params_gc_t params{};
  std::array<uint32_t, 32> value_list{};
  XCB_AUX_ADD_PARAM(&mask, &params, foreground, black_pixel);
  XCB_AUX_ADD_PARAM(&mask, &params, background, black_pixel);
  XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);
  connection::pack_values(mask, &params, value_list);
  m_gcontext = m_connection.generate_id();
  m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list.data());

  m_log.trace("background_manager: Allocating cairo surface");
  m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, visual, m_rect.width, m_rect.height);

  /*
   * Fill pixmap with black in case it is not fully filled by the root pixmap. Otherwise we may render uninitialized
   * memory
   */
  m_connection.poly_fill_rectangle(m_pixmap, m_gcontext, 1, &m_rect);
}

void bg_slice::free_resources() {
  m_surface.reset();

  if (m_pixmap != XCB_NONE) {
    m_connection.free_pixmap(m_pixmap);
    m_pixmap = XCB_NONE;
  }

  if (m_gcontext != XCB_NONE) {
    m_connection.free_gc(m_gcontext);
    m_gcontext = XCB_NONE;
  }
}

POLYBAR_NS_END

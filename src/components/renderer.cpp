#include "components/renderer.hpp"
#include "cairo/context.hpp"
#include "components/config.hpp"
#include "events/signal.hpp"
#include "events/signal_receiver.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

/**
 * Create instance
 */
renderer::make_type renderer::make(const bar_settings& bar) {
  // clang-format off
  return factory_util::unique<renderer>(
      connection::make(),
      signal_emitter::make(),
      config::make(),
      logger::make(),
      forward<decltype(bar)>(bar));
  // clang-format on
}

/**
 * Construct renderer instance
 */
renderer::renderer(
    connection& conn, signal_emitter& sig, const config& conf, const logger& logger, const bar_settings& bar)
    : m_connection(conn)
    , m_sig(sig)
    , m_conf(conf)
    , m_log(logger)
    , m_bar(forward<const bar_settings&>(bar))
    , m_rect(m_bar.inner_area()) {
  m_sig.attach(this);
  m_log.trace("renderer: Get TrueColor visual");
  {
    if ((m_visual = m_connection.visual_type(m_connection.screen(), 32)) == nullptr) {
      m_log.err("No 32-bit TrueColor visual found...");

      if ((m_visual = m_connection.visual_type(m_connection.screen(), 24)) == nullptr) {
        m_log.err("No 24-bit TrueColor visual found...");
      } else {
        m_depth = 24;
      }
    }
    if (m_visual == nullptr) {
      throw application_error("No matching TrueColor");
    }
  }

  m_log.trace("renderer: Allocate colormap");
  {
    m_colormap = m_connection.generate_id();
    m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, m_connection.screen()->root, m_visual->visual_id);
  }

  m_log.trace("renderer: Allocate output window");
  {
    // clang-format off
    m_window = winspec(m_connection)
      << cw_size(m_bar.size)
      << cw_pos(m_bar.pos)
      << cw_depth(m_depth)
      << cw_visual(m_visual->visual_id)
      << cw_class(XCB_WINDOW_CLASS_INPUT_OUTPUT)
      << cw_params_back_pixel(0)
      << cw_params_border_pixel(0)
      << cw_params_backing_store(XCB_BACKING_STORE_WHEN_MAPPED)
      << cw_params_colormap(m_colormap)
      << cw_params_event_mask(XCB_EVENT_MASK_PROPERTY_CHANGE
                             |XCB_EVENT_MASK_EXPOSURE
                             |XCB_EVENT_MASK_BUTTON_PRESS)
      << cw_params_override_redirect(m_bar.override_redirect)
      << cw_flush(true);
    // clang-format on
  }

  m_log.trace("renderer: Allocate window pixmaps");
  {
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap(m_depth, m_pixmap, m_window, m_bar.size.w, m_bar.size.h);
  }

  m_log.trace("renderer: Allocate graphic contexts");
  {
    unsigned int mask{0};
    unsigned int value_list[32]{0};
    xcb_params_gc_t params{};
    XCB_AUX_ADD_PARAM(&mask, &params, foreground, m_bar.foreground);
    XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);
    connection::pack_values(mask, &params, value_list);
    m_gcontext = m_connection.generate_id();
    m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list);
  }

  m_log.trace("renderer: Allocate alignment blocks");
  {
    const auto create_block = [&](alignment a) {
      auto pid = m_connection.generate_id();
      m_connection.create_pixmap_checked(m_depth, pid, m_pixmap, m_bar.size.w, m_bar.size.h);
      m_blocks.emplace(a, alignment_block{pid, 0.0, 0.0});
    };

    create_block(alignment::LEFT);
    create_block(alignment::CENTER);
    create_block(alignment::RIGHT);
  }

  m_log.trace("renderer: Allocate cairo components");
  {
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, m_visual, m_bar.size.w, m_bar.size.h);
    m_context = make_unique<cairo::context>(*m_surface, m_log);
  }

  m_log.trace("renderer: Load fonts");
  {
    double dpi_x = 96, dpi_y = 96;
    if (m_conf.has(m_conf.section(), "dpi")) {
      try {
        // dpi specified directly as a value
        dpi_x = dpi_y = m_conf.get<double>("dpi");
      } catch (const value_error&) {
        // dpi to be comptued
        auto screen = m_connection.screen();
        dpi_x = screen->width_in_pixels * 25.4 / screen->width_in_millimeters;
        dpi_y = screen->height_in_pixels * 25.4 / screen->height_in_millimeters;
      }
    }
    m_log.trace("renderer: DPI is %.1fx%.1f", dpi_x, dpi_y);

    auto fonts = m_conf.get_list<string>(m_conf.section(), "font", {});
    if (fonts.empty()) {
      m_log.warn("No fonts specified, using fallback font \"fixed\"");
      fonts.emplace_back("fixed");
    }

    auto fonts_loaded = false;
    for (const auto& f : fonts) {
      vector<string> fd{string_util::split(f, ';')};
      auto font = cairo::make_font(*m_context, string(fd[0]), fd.size() > 1 ? std::atoi(fd[1].c_str()) : 0, dpi_x, dpi_y);
      m_log.info("Loaded font \"%s\" (name=%s, file=%s)", fd[0], font->name(), font->file());
      *m_context << move(font);
      fonts_loaded = true;
    }

    if (!fonts_loaded) {
      throw application_error("Unable to load fonts");
    }
  }

  m_comp_bg = cairo::utils::str2operator(m_conf.get("settings", "compositing-background", ""s), CAIRO_OPERATOR_OVER);
  m_comp_fg = cairo::utils::str2operator(m_conf.get("settings", "compositing-foreground", ""s), CAIRO_OPERATOR_OVER);
  m_comp_ol = cairo::utils::str2operator(m_conf.get("settings", "compositing-overline", ""s), CAIRO_OPERATOR_OVER);
  m_comp_ul = cairo::utils::str2operator(m_conf.get("settings", "compositing-underline", ""s), CAIRO_OPERATOR_OVER);
  m_comp_border = cairo::utils::str2operator(m_conf.get("settings", "compositing-border", ""s), CAIRO_OPERATOR_OVER);

  m_fixedcenter = m_conf.get(m_conf.section(), "fixed-center", true);
}

/**
 * Deconstruct instance
 */
renderer::~renderer() {
  m_sig.detach(this);
}

/**
 * Get output window
 */
xcb_window_t renderer::window() const {
  return m_window;
}

/**
 * Get completed action blocks
 */
const vector<action_block> renderer::actions() const {
  return m_actions;
}

/**
 * Begin render routine
 */
void renderer::begin() {
  m_log.trace_x("renderer: begin");

  // Reset state
  m_actions.clear();
  m_attr.reset();
  m_align = alignment::NONE;
  m_rect = m_bar.inner_area();

  // Reset colors
  m_bg = 0;
  m_fg = m_bar.foreground;
  m_ul = m_bar.underline.color;
  m_ol = m_bar.overline.color;

  m_context->save();

  // Clear canvas
  m_surface->set_drawable(m_pixmap, m_bar.size.w, m_bar.size.h);
  m_context->clear();

  fill_borders();

  // clang-format off
  m_context->clip(cairo::rect{
    static_cast<double>(m_rect.x),
    static_cast<double>(m_rect.y),
    static_cast<double>(m_rect.width),
    static_cast<double>(m_rect.height)});
  // clang-format on

  fill_background();
}

/**
 * End render routine
 */
void renderer::end() {
  m_log.trace_x("renderer: end");

  for (auto&& b : m_blocks) {
    if (block_x(b.first) + block_w(b.first) > (m_rect.width)) {
      double x = m_rect.x + block_x(b.first) + block_w(b.first);
      if ((x -= x - m_rect.x - m_rect.width + block_x(b.first)) > 0.0) {
        // clang-format off
        rgba bg1{m_bar.background}; bg1.a = 0.0;
        rgba bg2{m_bar.background}; bg2.a = 1.0;
        // clang-format on

        m_surface->set_drawable(b.second.pixmap, m_bar.size.w, m_bar.size.h);
        m_context->save();
        *m_context << cairo::linear_gradient{x - 40.0, 0.0, x, 0.0, {bg1, bg2}};
        m_context->paint();
        m_context->restore();
      }
    }
  }

  for (auto&& a : m_actions) {
    a.start_x += block_x(a.align) + m_rect.x;
    a.end_x += block_x(a.align) + m_rect.x;
  }

  m_context->restore();

  flush();
}

/**
 * Flush pixmap contents onto the target window
 */
void renderer::flush() {
  m_log.trace_x("renderer: flush");

  m_surface->set_drawable(m_pixmap, m_bar.size.w, m_bar.size.h);
  m_surface->flush();

  for (auto&& b : m_blocks) {
    double x = m_rect.x + block_x(b.first);
    double y = m_rect.y + block_y(b.first);
    double w = m_rect.x + m_rect.width - x;
    double h = m_rect.y + m_rect.height - y;

    if (w > 0.0) {
      m_log.trace_x("renderer: copy alignment block (x=%.0f y=%.0f w=%.0f h=%.0f)", x, y, w, h);
      m_connection.copy_area(b.second.pixmap, m_pixmap, m_gcontext, m_rect.x, m_rect.y, x, y, w, h);
      m_connection.flush();
    } else {
      m_log.trace_x("renderer: ignoring empty alignment block");
      continue;
    }
  }

  m_surface->dirty();
  m_surface->flush();

  highlight_clickable_areas();

#if 0
#ifdef DEBUG_SHADED
  if (m_bar.shaded && m_bar.origin == edge::TOP) {
    m_log.trace_x(
        "renderer: copy pixmap (shaded=1, geom=%dx%d+%d+%d)", m_rect.width, m_rect.height, m_rect.x, m_rect.y);
    auto geom = m_connection.get_geometry(m_window);
    auto x1 = 0;
    auto y1 = m_rect.height - m_bar.shade_size.h - m_rect.y - geom->height;
    auto x2 = m_rect.x;
    auto y2 = m_rect.y;
    auto w = m_rect.width;
    auto h = m_rect.height - m_bar.shade_size.h + geom->height;
    m_connection.copy_area(m_pixmap, m_window, m_gcontext, x1, y1, x2, y2, w, h);
    m_connection.flush();
    return;
  }
#endif
#endif

  m_connection.copy_area(m_pixmap, m_window, m_gcontext, 0, 0, 0, 0, m_bar.size.w, m_bar.size.h);
  m_connection.flush();

  if (!m_snapshot_dst.empty()) {
    try {
      m_surface->write_png(m_snapshot_dst);
      m_log.info("Successfully wrote %s", m_snapshot_dst);
    } catch (const exception& err) {
      m_log.err("Failed to write snapshot (err: %s)", err.what());
    }
    m_snapshot_dst.clear();
  }
}

/**
 * Get x position of block for given alignment
 */
double renderer::block_x(alignment a) const {
  switch (a) {
    case alignment::CENTER:
      if (!m_fixedcenter || m_rect.width / 2.0 + block_w(a) / 2.0 > m_rect.width - block_w(alignment::RIGHT)) {
        return std::max((m_rect.width - block_w(alignment::RIGHT) + block_w(alignment::LEFT)) / 2.0 - block_w(a) / 2.0,
            block_w(alignment::LEFT));
      } else {
        return std::max(m_rect.width / 2.0 - block_w(a) / 2.0, block_w(alignment::LEFT));
      }
    case alignment::RIGHT:
      return std::max(m_rect.width - block_w(a), block_x(alignment::CENTER) + block_w(alignment::CENTER));
    default:
      return 0.0;
  }
}

/**
 * Get y position of block for given alignment
 */
double renderer::block_y(alignment) const {
  return 0.0;
}

/**
 * Get block width for given alignment
 */
double renderer::block_w(alignment a) const {
  return m_blocks.at(a).x;
}

/**
 * Reserve space at given edge
 */
void renderer::reserve_space(edge side, unsigned int w) {
  m_log.trace_x("renderer: reserve_space(%i, %i)", static_cast<int>(side), w);

  m_cleararea.side = side;
  m_cleararea.size = w;

  switch (side) {
    case edge::NONE:
      break;
    case edge::TOP:
      m_rect.y += w;
      m_rect.height -= w;
      break;
    case edge::BOTTOM:
      m_rect.height -= w;
      break;
    case edge::LEFT:
      m_rect.x += w;
      m_rect.width -= w;
      break;
    case edge::RIGHT:
      m_rect.width -= w;
      break;
    case edge::ALL:
      m_rect.x += w;
      m_rect.y += w;
      m_rect.width -= w * 2;
      m_rect.height -= w * 2;
      break;
  }
}

/**
 * Fill background color
 */
void renderer::fill_background() {
  m_context->save();
  *m_context << static_cast<cairo_operator_t>(m_comp_bg);

  if (m_bar.radius != 0.0) {
    // clang-format off
    *m_context << cairo::rounded_corners{
        static_cast<double>(m_rect.x),
        static_cast<double>(m_rect.y),
        static_cast<double>(m_rect.width),
        static_cast<double>(m_rect.height), m_bar.radius};
    // clang-format on
  }

  if (!m_bar.background_steps.empty()) {
    m_log.trace_x("renderer: gradient background (steps=%lu)", m_bar.background_steps.size());
    *m_context << cairo::linear_gradient{0.0, 0.0 + m_rect.y, 0.0, 0.0 + m_rect.height, m_bar.background_steps};
  } else {
    m_log.trace_x("renderer: solid background #%08x", m_bar.background);
    *m_context << m_bar.background;
  }

  if (m_bar.radius != 0.0) {
    m_context->fill();
  } else {
    m_context->paint();
  }

  m_context->restore();
}

/**
 * Fill overline color
 */
void renderer::fill_overline(double x, double w) {
  if (m_bar.overline.size && m_attr.test(static_cast<int>(attribute::OVERLINE))) {
    m_log.trace_x("renderer: overline(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << static_cast<cairo_operator_t>(m_comp_ol);
    *m_context << m_ol;
    *m_context << cairo::rect{x, static_cast<double>(m_rect.y), w, static_cast<double>(m_bar.overline.size)};
    m_context->fill();
    m_context->restore();
  }
}

/**
 * Fill underline color
 */
void renderer::fill_underline(double x, double w) {
  if (m_bar.underline.size && m_attr.test(static_cast<int>(attribute::UNDERLINE))) {
    m_log.trace_x("renderer: underline(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << static_cast<cairo_operator_t>(m_comp_ul);
    *m_context << m_ul;
    *m_context << cairo::rect{x, static_cast<double>(m_rect.y + m_rect.height - m_bar.underline.size), w,
        static_cast<double>(m_bar.underline.size)};
    m_context->fill();
    m_context->restore();
  }
}

/**
 * Fill border colors
 */
void renderer::fill_borders() {
  m_context->save();
  *m_context << static_cast<cairo_operator_t>(m_comp_border);

  cairo::rect top{0.0, 0.0, 0.0, 0.0};
  top.x += m_bar.borders.at(edge::LEFT).size;
  top.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::TOP).size;
  top.h += m_bar.borders.at(edge::TOP).size;
  m_log.trace_x("renderer: border T(%.0f, #%08x)", top.h, m_bar.borders.at(edge::TOP).color);
  (*m_context << top << m_bar.borders.at(edge::TOP).color).fill();

  cairo::rect bottom{0.0, 0.0, 0.0, 0.0};
  bottom.x += m_bar.borders.at(edge::LEFT).size;
  bottom.y += m_bar.size.h - m_bar.borders.at(edge::BOTTOM).size;
  bottom.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
  bottom.h += m_bar.borders.at(edge::BOTTOM).size;
  m_log.trace_x("renderer: border B(%.0f, #%08x)", bottom.h, m_bar.borders.at(edge::BOTTOM).color);
  (*m_context << bottom << m_bar.borders.at(edge::BOTTOM).color).fill();

  cairo::rect left{0.0, 0.0, 0.0, 0.0};
  left.w += m_bar.borders.at(edge::LEFT).size;
  left.h += m_bar.size.h;
  m_log.trace_x("renderer: border L(%.0f, #%08x)", left.w, m_bar.borders.at(edge::LEFT).color);
  (*m_context << left << m_bar.borders.at(edge::LEFT).color).fill();

  cairo::rect right{0.0, 0.0, 0.0, 0.0};
  right.x += m_bar.size.w - m_bar.borders.at(edge::RIGHT).size;
  right.w += m_bar.borders.at(edge::RIGHT).size;
  right.h += m_bar.size.h;
  m_log.trace_x("renderer: border R(%.0f, #%08x)", right.w, m_bar.borders.at(edge::RIGHT).color);
  (*m_context << right << m_bar.borders.at(edge::RIGHT).color).fill();

  m_context->restore();
}

/**
 * Draw text contents
 */
void renderer::draw_text(const string& contents) {
  m_log.trace_x("renderer: text(%s)", contents.c_str());

  cairo::abspos origin{};
  origin.x = m_rect.x + m_blocks[m_align].x;
  origin.y = m_rect.y + m_rect.height / 2.0;

  cairo::textblock block{};
  block.align = m_align;
  block.contents = contents;
  block.font = m_font;
  block.bg = 0;
  block.bg_rect = cairo::rect{0.0, 0.0, 0.0, 0.0};
  block.bg_operator = 0;
  block.x_advance = &m_blocks[m_align].x;
  block.y_advance = &m_blocks[m_align].y;

  if (m_bg && m_bg != m_bar.background) {
    block.bg = m_bg;
    block.bg_operator = static_cast<cairo_operator_t>(m_comp_bg);
    block.bg_rect.y = m_rect.y;
    block.bg_rect.h = m_rect.height;
  }

  m_context->save();
  *m_context << origin;
  *m_context << static_cast<cairo_operator_t>(m_comp_fg);
  *m_context << m_fg;
  *m_context << block;
  m_context->restore();

  double dx = m_rect.x + m_blocks[m_align].x - origin.x;
  if (dx > 0.0) {
    fill_underline(origin.x, dx);
    fill_overline(origin.x, dx);
  }
}

/**
 * Colorize the bounding box of created action blocks
 */
void renderer::highlight_clickable_areas() {
#ifdef DEBUG_HINTS
  map<alignment, int> hint_num{};
  for (auto&& action : m_actions) {
    if (!action.active) {
      int n = hint_num.find(action.align)->second++;
      double x = action.start_x;
      double y = m_rect.y;
      double w = action.width();
      double h = m_rect.height;

      m_context->save();
      *m_context << CAIRO_OPERATOR_DIFFERENCE << (n % 2 ? 0xFF00FF00 : 0xFFFF0000);
      *m_context << cairo::rect{x, y, w, h};
      m_context->fill();
      m_context->restore();
    }
  }
  m_surface->flush();
#endif
}

bool renderer::on(const signals::ui::request_snapshot& evt) {
  m_snapshot_dst = evt.cast();
  return true;
}

bool renderer::on(const signals::parser::change_background& evt) {
  const unsigned int color{evt.cast()};
  if (color != m_bg) {
    m_log.trace_x("renderer: change_background(#%08x)", color);
    m_bg = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_foreground& evt) {
  const unsigned int color{evt.cast()};
  if (color != m_fg) {
    m_log.trace_x("renderer: change_foreground(#%08x)", color);
    m_fg = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_underline& evt) {
  const unsigned int color{evt.cast()};
  if (color != m_ul) {
    m_log.trace_x("renderer: change_underline(#%08x)", color);
    m_ul = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_overline& evt) {
  const unsigned int color{evt.cast()};
  if (color != m_ol) {
    m_log.trace_x("renderer: change_overline(#%08x)", color);
    m_ol = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_font& evt) {
  const int font{evt.cast()};
  if (font != m_font) {
    m_log.trace_x("renderer: change_font(%i)", font);
    m_font = font;
  }
  return true;
}

bool renderer::on(const signals::parser::change_alignment& evt) {
  auto align = static_cast<const alignment&>(evt.cast());
  if (align != m_align) {
    m_log.trace_x("renderer: change_alignment(%i)", static_cast<int>(align));
    m_align = align;
    m_blocks[m_align].x = 0.0;
    m_blocks[m_align].y = 0.0;
    m_surface->set_drawable(m_blocks.at(m_align).pixmap, m_bar.size.w, m_bar.size.h);
    m_context->clear();
    fill_background();
  }
  return true;
}

bool renderer::on(const signals::parser::offset_pixel& evt) {
  m_log.trace_x("renderer: offset_pixel(%f)", evt.cast());
  m_blocks[m_align].x += evt.cast();
  return true;
}

bool renderer::on(const signals::parser::attribute_set& evt) {
  m_log.trace_x("renderer: attribute_set(%i)", static_cast<int>(evt.cast()));
  m_attr.set(static_cast<int>(evt.cast()), true);
  return true;
}

bool renderer::on(const signals::parser::attribute_unset& evt) {
  m_log.trace_x("renderer: attribute_unset(%i)", static_cast<int>(evt.cast()));
  m_attr.set(static_cast<int>(evt.cast()), false);
  return true;
}

bool renderer::on(const signals::parser::attribute_toggle& evt) {
  m_log.trace_x("renderer: attribute_toggle(%i)", static_cast<int>(evt.cast()));
  m_attr.flip(static_cast<int>(evt.cast()));
  return true;
}

bool renderer::on(const signals::parser::action_begin& evt) {
  auto a = evt.cast();
  m_log.trace_x("renderer: action_begin(btn=%i, command=%s)", static_cast<int>(a.button), a.command);
  action_block action{};
  action.button = a.button == mousebtn::NONE ? mousebtn::LEFT : a.button;
  action.align = m_align;
  action.start_x = m_blocks.at(m_align).x;
  action.command = string_util::replace_all(a.command, ":", "\\:");
  action.active = true;
  m_actions.emplace_back(action);
  return true;
}

bool renderer::on(const signals::parser::action_end& evt) {
  auto btn = evt.cast();
  m_log.trace_x("renderer: action_end(btn=%i)", static_cast<int>(btn));
  for (auto action = m_actions.rbegin(); action != m_actions.rend(); action++) {
    if (action->active && action->align == m_align && action->button == btn) {
      action->end_x = m_blocks.at(action->align).x;
      action->active = false;
    }
  }
  return true;
}

bool renderer::on(const signals::parser::text& evt) {
  auto text = evt.cast();
  draw_text(text);
  return true;
}

POLYBAR_NS_END

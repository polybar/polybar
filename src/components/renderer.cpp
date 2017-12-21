#include "components/renderer.hpp"
#include "cairo/context.hpp"
#include "components/config.hpp"
#include "events/signal.hpp"
#include "events/signal_receiver.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"
#include "x11/atoms.hpp"
#include "x11/background_manager.hpp"
#include "x11/connection.hpp"
#include "x11/extensions/all.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

static constexpr double BLOCK_GAP{20.0};

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
      forward<decltype(bar)>(bar),
      background_manager::make());
  // clang-format on
}

/**
 * Construct renderer instance
 */
renderer::renderer(
    connection& conn, signal_emitter& sig, const config& conf, const logger& logger, const bar_settings& bar, background_manager& background)
    : m_connection(conn)
    , m_sig(sig)
    , m_conf(conf)
    , m_log(logger)
    , m_bar(forward<const bar_settings&>(bar))
    , m_background(background)
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
    m_blocks.emplace(alignment::LEFT, alignment_block{nullptr, 0.0, 0.0});
    m_blocks.emplace(alignment::CENTER, alignment_block{nullptr, 0.0, 0.0});
    m_blocks.emplace(alignment::RIGHT, alignment_block{nullptr, 0.0, 0.0});
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
      dpi_x = dpi_y = m_conf.get<double>("dpi");
    } else {
      if (m_conf.has(m_conf.section(), "dpi-x")) {
        dpi_x = m_conf.get<double>("dpi-x");
      }
      if (m_conf.has(m_conf.section(), "dpi-y")) {
        dpi_y = m_conf.get<double>("dpi-y");
      }
    }

    // dpi to be comptued
    if (dpi_x <= 0 || dpi_y <= 0) {
      auto screen = m_connection.screen();
      if (dpi_x <= 0) {
        dpi_x = screen->width_in_pixels * 25.4 / screen->width_in_millimeters;
      }
      if (dpi_y <= 0) {
        dpi_y = screen->height_in_pixels * 25.4 / screen->height_in_millimeters;
      }
    }

    m_log.info("Configured DPI = %gx%g", dpi_x, dpi_y);

    auto fonts = m_conf.get_list<string>(m_conf.section(), "font", {});
    if (fonts.empty()) {
      m_log.warn("No fonts specified, using fallback font \"fixed\"");
      fonts.emplace_back("fixed");
    }

    for (const auto& f : fonts) {
      int offset{0};
      string pattern{f};
      size_t pos = pattern.rfind(';');
      if (pos != string::npos) {
        offset = std::strtol(pattern.substr(pos + 1).c_str(), nullptr, 10);
        pattern.erase(pos);
      }
      auto font = cairo::make_font(*m_context, string{pattern}, offset, dpi_x, dpi_y);
      m_log.info("Loaded font \"%s\" (name=%s, offset=%i, file=%s)", pattern, font->name(), offset, font->file());
      *m_context << move(font);
    }

    m_log.trace("Activate root background manager");
    m_background.activate(m_window, m_bar.outer_area(false));
  }

  m_comp_bg = m_conf.get<cairo_operator_t>("settings", "compositing-background", m_comp_bg);
  m_comp_fg = m_conf.get<cairo_operator_t>("settings", "compositing-foreground", m_comp_fg);
  m_comp_ol = m_conf.get<cairo_operator_t>("settings", "compositing-overline", m_comp_ol);
  m_comp_ul = m_conf.get<cairo_operator_t>("settings", "compositing-underline", m_comp_ul);
  m_comp_border = m_conf.get<cairo_operator_t>("settings", "compositing-border", m_comp_border);

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
void renderer::begin(xcb_rectangle_t rect) {
  m_log.trace_x("renderer: begin (geom=%ix%i+%i+%i)", rect.width, rect.height, rect.x, rect.y);

  // Reset state
  m_rect = rect;
  m_actions.clear();
  m_attr.reset();
  m_align = alignment::NONE;

  // Reset colors
  m_bg = 0x0;
  m_fg = m_bar.foreground;
  m_ul = m_bar.underline.color;
  m_ol = m_bar.overline.color;

  // Clear canvas
  m_context->save();
  m_context->clear();

  // Draw the background as base layer so that everything
  // else is drawn on top of it
  fill_background();


  // Create corner mask
  if (m_bar.radius && m_cornermask == nullptr) {
    m_context->save();
    m_context->push();
    // clang-format off
    *m_context << cairo::rounded_corners{
        static_cast<double>(m_rect.x),
        static_cast<double>(m_rect.y),
        static_cast<double>(m_rect.width),
        static_cast<double>(m_rect.height), m_bar.radius};
    // clang-format on
    *m_context << rgba{1.0, 1.0, 1.0, 1.0};
    m_context->fill();
    m_context->pop(&m_cornermask);
    m_context->restore();
  }

  fill_borders();

  // clang-format off
  m_context->clip(cairo::rect{
      static_cast<double>(m_rect.x),
      static_cast<double>(m_rect.y),
      static_cast<double>(m_rect.width),
      static_cast<double>(m_rect.height)});
  // clang-format on
}

/**
 * End render routine
 */
void renderer::end() {
  m_log.trace_x("renderer: end");

  for (auto&& a : m_actions) {
    a.start_x += block_x(a.align) + m_rect.x;
    a.end_x += block_x(a.align) + m_rect.x;
  }

  if (m_align != alignment::NONE) {
    m_log.trace_x("renderer: pop(%i)", static_cast<int>(m_align));
    m_context->pop(&m_blocks[m_align].pattern);

    // Capture the concatenated block contents
    // so that it can be masked with the corner pattern
    m_context->push();

    for (auto&& b : m_blocks) {
      flush(b.first);
    }

    cairo_pattern_t* blockcontents{};
    m_context->pop(&blockcontents);

    if (m_cornermask != nullptr) {
      *m_context << blockcontents;
      m_context->mask(m_cornermask);
    } else {
      *m_context << blockcontents;
      m_context->paint();
    }

    m_context->destroy(&blockcontents);
  } else {
    fill_background();
  }

  m_context->restore();
  m_surface->flush();

  flush();

  m_sig.emit(signals::ui::changed{});
}

/**
 * Flush contents of given alignment block
 */
void renderer::flush(alignment a) {
  if (m_blocks[a].pattern == nullptr) {
    return;
  }

  m_context->save();

  double x = static_cast<int>(block_x(a) + 0.5);
  double y = static_cast<int>(block_y(a) + 0.5);
  double w = static_cast<int>(block_w(a) + 0.5);
  double h = static_cast<int>(block_h(a) + 0.5);
  double xw = x + w;
  bool fits{xw <= m_rect.x + m_rect.width};

  m_log.trace("renderer: flush(%i geom=%gx%g+%g+%g, falloff=%i)", static_cast<int>(a), w, h, x, y, !fits);

  // Set block shape
  *m_context << cairo::abspos{0.0, 0.0};
  *m_context << cairo::rect{m_rect.x + x, m_rect.y + y, w, h};

  // Restrict drawing to the block rectangle
  m_context->clip(true);

  // Clear the area covered by the block
  m_context->clear();

  *m_context << cairo::translate{x, 0.0};
  *m_context << m_blocks[a].pattern;
  m_context->paint();

  if (!fits) {
    // Paint falloff gradient at the end of the visible block
    // to indicate that the content expands past the canvas
    double fx = w - (xw - m_rect.width);
    double fsize = std::max(5.0, std::min(std::abs(fx), 30.0));
    m_log.trace("renderer: Drawing falloff (pos=%g, size=%g)", fx, fsize);
    *m_context << cairo::linear_gradient{fx - fsize, 0.0, fx, 0.0, {0x00000000, 0xFF000000}};
    m_context->paint(0.25);
  }

  *m_context << cairo::abspos{0.0, 0.0};
  m_context->destroy(&m_blocks[a].pattern);
  m_context->restore();
}

/**
 * Flush pixmap contents onto the target window
 */
void renderer::flush() {
  m_log.trace_x("renderer: flush");

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

  m_surface->flush();
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
    case alignment::CENTER: {
      double base_pos{0.0};
      double min_pos{0.0};
      if (!m_fixedcenter || m_rect.width / 2.0 + block_w(a) / 2.0 > m_rect.width - block_w(alignment::RIGHT)) {
        base_pos = (m_rect.width - block_w(alignment::RIGHT) + block_w(alignment::LEFT)) / 2.0;
      } else {
        base_pos = m_rect.width / 2.0;
      }
      if ((min_pos = block_w(alignment::LEFT))) {
        min_pos += BLOCK_GAP;
      }

      base_pos += (m_bar.size.w - m_rect.width) / 2.0;

      int border_left = m_bar.borders.at(edge::LEFT).size;

      /*
       * If m_rect.x is greater than the left border, then the systray is rendered on the left and we need to adjust for
       * that.
       * Since we cannot access any tray objects from here we need to calculate the tray size through m_rect.x
       * m_rect.x is the x-position of the bar (without the tray or any borders), so if the tray is on the left,
       * m_rect.x effectively is border_left + tray_width.
       * So we can just subtract the tray_width = m_rect.x - border_left from the base_pos to correct for the tray being
       * placed on the left
       */
      if(m_rect.x > border_left) {
        base_pos -= m_rect.x - border_left;
      }

      base_pos -= border_left;

      return std::max(base_pos - block_w(a) / 2.0, min_pos);
    }
    case alignment::RIGHT: {
      double gap{0.0};
      if (block_w(alignment::LEFT) || block_w(alignment::CENTER)) {
        gap = BLOCK_GAP;
      }
      return std::max(m_rect.width - block_w(a), block_x(alignment::CENTER) + gap + block_w(alignment::CENTER));
    }
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
 * Get block height for given alignment
 */
double renderer::block_h(alignment) const {
  return m_rect.height;
}

#if 0
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
#endif

/**
 * Fill background color
 */
void renderer::fill_background() {
  m_context->save();
  *m_context << m_comp_bg;

  auto root_bg = m_background.get_surface();
  if(root_bg != nullptr) {
    m_log.trace_x("renderer: root background");
    *m_context << *root_bg;
    m_context->paint();
    *m_context << CAIRO_OPERATOR_OVER;
  }

  if (!m_bar.background_steps.empty()) {
    m_log.trace_x("renderer: gradient background (steps=%lu)", m_bar.background_steps.size());
    *m_context << cairo::linear_gradient{0.0, 0.0 + m_rect.y, 0.0, 0.0 + m_rect.height, m_bar.background_steps};
  } else {
    m_log.trace_x("renderer: solid background #%08x", m_bar.background);
    *m_context << m_bar.background;
  }

  m_context->paint();
  m_context->restore();
}

/**
 * Fill overline color
 */
void renderer::fill_overline(double x, double w) {
  if (m_bar.overline.size && m_attr.test(static_cast<int>(attribute::OVERLINE))) {
    m_log.trace_x("renderer: overline(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << m_comp_ol;
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
    *m_context << m_comp_ul;
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
  *m_context << m_comp_border;

  if (m_bar.borders.at(edge::TOP).size) {
    cairo::rect top{0.0, 0.0, 0.0, 0.0};
    top.x += m_bar.borders.at(edge::LEFT).size;
    top.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
    top.h += m_bar.borders.at(edge::TOP).size;
    m_log.trace_x("renderer: border T(%.0f, #%08x)", top.h, m_bar.borders.at(edge::TOP).color);
    (*m_context << top << m_bar.borders.at(edge::TOP).color).fill();
  }

  if (m_bar.borders.at(edge::BOTTOM).size) {
    cairo::rect bottom{0.0, 0.0, 0.0, 0.0};
    bottom.x += m_bar.borders.at(edge::LEFT).size;
    bottom.y += m_bar.size.h - m_bar.borders.at(edge::BOTTOM).size;
    bottom.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
    bottom.h += m_bar.borders.at(edge::BOTTOM).size;
    m_log.trace_x("renderer: border B(%.0f, #%08x)", bottom.h, m_bar.borders.at(edge::BOTTOM).color);
    (*m_context << bottom << m_bar.borders.at(edge::BOTTOM).color).fill();
  }

  if (m_bar.borders.at(edge::LEFT).size) {
    cairo::rect left{0.0, 0.0, 0.0, 0.0};
    left.w += m_bar.borders.at(edge::LEFT).size;
    left.h += m_bar.size.h;
    m_log.trace_x("renderer: border L(%.0f, #%08x)", left.w, m_bar.borders.at(edge::LEFT).color);
    (*m_context << left << m_bar.borders.at(edge::LEFT).color).fill();
  }

  if (m_bar.borders.at(edge::RIGHT).size) {
    cairo::rect right{0.0, 0.0, 0.0, 0.0};
    right.x += m_bar.size.w - m_bar.borders.at(edge::RIGHT).size;
    right.w += m_bar.borders.at(edge::RIGHT).size;
    right.h += m_bar.size.h;
    m_log.trace_x("renderer: border R(%.0f, #%08x)", right.w, m_bar.borders.at(edge::RIGHT).color);
    (*m_context << right << m_bar.borders.at(edge::RIGHT).color).fill();
  }

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
  block.x_advance = &m_blocks[m_align].x;
  block.y_advance = &m_blocks[m_align].y;
  block.bg_rect = cairo::rect{0.0, 0.0, 0.0, 0.0};

  // Only draw text background if the color differs from
  // the background color of the bar itself
  // Note: this means that if the user explicitly set text
  // background color equal to background-0 it will be ignored
  if (m_bg != m_bar.background) {
    block.bg = m_bg;
    block.bg_operator = m_comp_bg;
    block.bg_rect.x = m_rect.x;
    block.bg_rect.y = m_rect.y;
    block.bg_rect.h = m_rect.height;
  }

  m_context->save();
  *m_context << origin;
  *m_context << m_comp_fg;
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

    if (m_align != alignment::NONE) {
      m_log.trace_x("renderer: pop(%i)", static_cast<int>(m_align));
      m_context->pop(&m_blocks[m_align].pattern);
    }

    m_align = align;
    m_blocks[m_align].x = 0.0;
    m_blocks[m_align].y = 0.0;
    m_context->push();
    m_log.trace_x("renderer: push(%i)", static_cast<int>(m_align));
  }
  return true;
}

bool renderer::on(const signals::parser::reverse_colors&) {
  m_log.trace_x("renderer: reverse_colors");
  m_fg = m_fg + m_bg;
  m_bg = m_fg - m_bg;
  m_fg = m_fg - m_bg;
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
  action.command = string_util::replace_all(a.command, "\\:", ":");
  action.active = true;
  m_actions.emplace_back(action);
  return true;
}

bool renderer::on(const signals::parser::action_end& evt) {
  auto btn = evt.cast();

  /*
   * Iterate actions in reverse and find the FIRST active action that matches
   */
  m_log.trace_x("renderer: action_end(btn=%i)", static_cast<int>(btn));
  for (auto action = m_actions.rbegin(); action != m_actions.rend(); action++) {
    if (action->active && action->align == m_align && action->button == btn) {
      action->end_x = m_blocks.at(action->align).x;
      action->active = false;
      break;
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

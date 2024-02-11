#include "components/renderer.hpp"

#include <cassert>

#include "cairo/context.hpp"
#include "components/config.hpp"
#include "events/signal.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_receiver.hpp"
#include "utils/math.hpp"
#include "utils/units.hpp"
#include "x11/atoms.hpp"
#include "x11/background_manager.hpp"
#include "x11/connection.hpp"
#include "x11/winspec.hpp"

POLYBAR_NS

static constexpr double BLOCK_GAP{20.0};

/**
 * Create instance
 */
renderer::make_type renderer::make(const bar_settings& bar, tags::action_context& action_ctxt, const config& conf) {
  // clang-format off
  return std::make_unique<renderer>(
      connection::make(),
      signal_emitter::make(),
      conf,
      logger::make(),
      forward<decltype(bar)>(bar),
      background_manager::make(),
      action_ctxt);
  // clang-format on
}

/**
 * Construct renderer instance
 */
renderer::renderer(connection& conn, signal_emitter& sig, const config& conf, const logger& logger,
    const bar_settings& bar, background_manager& background, tags::action_context& action_ctxt)
    : renderer_interface(action_ctxt)
    , m_connection(conn)
    , m_sig(sig)
    , m_conf(conf)
    , m_log(logger)
    , m_bar(forward<const bar_settings&>(bar))
    , m_rect(m_bar.inner_area()) {
  m_sig.attach(this);

  m_log.trace("renderer: Get TrueColor visual");
  if ((m_visual = m_connection.visual_type(XCB_VISUAL_CLASS_TRUE_COLOR, 32)) != nullptr) {
    m_depth = 32;
  } else if ((m_visual = m_connection.visual_type(XCB_VISUAL_CLASS_TRUE_COLOR, 24)) != nullptr) {
    m_depth = 24;
  } else {
    throw application_error("Could not find a 24 or 32-bit TrueColor visual");
  }

  m_log.info("renderer: Using %d-bit TrueColor visual: 0x%x", m_depth, m_visual->visual_id);

  m_log.trace("renderer: Allocate colormap");
  m_colormap = m_connection.generate_id();
  m_connection.create_colormap(XCB_COLORMAP_ALLOC_NONE, m_colormap, m_connection.root(), m_visual->visual_id);

  m_log.trace("renderer: Allocate output window");
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

  m_log.trace("renderer: Allocate window pixmaps");
  {
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap(m_depth, m_pixmap, m_window, m_bar.size.w, m_bar.size.h);
  }

  m_log.trace("renderer: Allocate graphic contexts");
  {
    uint32_t mask{0};
    std::array<uint32_t, 32> value_list{};
    xcb_params_gc_t params{};
    XCB_AUX_ADD_PARAM(&mask, &params, foreground, m_bar.foreground);
    XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);
    connection::pack_values(mask, &params, value_list);
    m_gcontext = m_connection.generate_id();
    m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list.data());
  }

  m_log.trace("renderer: Allocate alignment blocks");
  {
    m_blocks.emplace(alignment::LEFT, alignment_block{nullptr, 0.0, 0.0, 0.});
    m_blocks.emplace(alignment::CENTER, alignment_block{nullptr, 0.0, 0.0, 0.});
    m_blocks.emplace(alignment::RIGHT, alignment_block{nullptr, 0.0, 0.0, 0.});
  }

  m_log.trace("renderer: Allocate cairo components");
  {
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, m_visual, m_bar.size.w, m_bar.size.h);
    m_context = make_unique<cairo::context>(*m_surface, m_log);
  }

  m_log.trace("renderer: Load fonts");
  {
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
      auto font = cairo::make_font(*m_context, string{pattern}, offset, m_bar.dpi_x, m_bar.dpi_y);
      m_log.notice("Loaded font \"%s\" (name=%s, offset=%i, file=%s)", pattern, font->name(), offset, font->file());
      *m_context << move(font);
    }
  }

  m_pseudo_transparency = m_conf.get<bool>("settings", "pseudo-transparency", m_pseudo_transparency);
  if (m_pseudo_transparency) {
    m_log.trace("Activate root background manager");
    m_background = background.observe(m_bar.outer_area(false), m_window);
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
 * Get bar window
 */
xcb_window_t renderer::window() const {
  return m_window;
}

/**
 * Get the bar window visual
 */
xcb_visualtype_t* renderer::visual() const {
  return m_visual;
}

/**
 * Get the bar window depth
 */
int renderer::depth() const {
  return m_depth;
}

/**
 * Begin render routine
 */
void renderer::begin(xcb_rectangle_t rect) {
  m_log.trace_x("renderer: begin (geom=%ix%i+%i+%i)", rect.width, rect.height, rect.x, rect.y);

  // Reset state
  m_rect = rect;
  m_align = alignment::NONE;

  // Clear canvas
  m_context->save();
  m_context->clear();

  // when pseudo-transparency is requested, render the bar into a new layer
  // that will later be composited against the desktop background
  if (m_pseudo_transparency) {
    m_context->push();
  }

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
    *m_context << rgba{0xffffffff};
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

  if (m_align != alignment::NONE) {
    m_log.trace_x("renderer: pop(%i)", static_cast<int>(m_align));
    m_context->pop(&m_blocks[m_align].pattern);

    // Capture the concatenated block contents
    // so that it can be masked with the corner pattern
    m_context->push();

    // Draw the background on the new layer to make up for
    // the areas not covered by the alignment blocks
    fill_background();

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

  // For pseudo-transparency, capture the contents of the rendered bar and
  // composite it against the desktop wallpaper. This way transparent parts of
  // the bar will be filled by the wallpaper creating illusion of transparency.
  if (m_pseudo_transparency) {
    cairo_pattern_t* barcontents{};
    m_context->pop(&barcontents); // corresponding push is in renderer::begin

    auto root_bg = m_background->get_surface();
    if (root_bg != nullptr) {
      m_log.trace_x("renderer: root background");
      *m_context << *root_bg;
      m_context->paint();
      *m_context << CAIRO_OPERATOR_OVER;
    }
    *m_context << barcontents;
    m_context->paint();
    m_context->destroy(&barcontents);
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
  bool fits{xw <= m_rect.width};

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

  *m_context << cairo::abspos{0.0, 0.0};
  m_context->destroy(&m_blocks[a].pattern);
  m_context->restore();

  if (!fits) {
    // Paint falloff gradient at the end of the visible block
    // to indicate that the content expands past the canvas

    /*
     * How many pixels are hidden
     */
    double overflow = xw - m_rect.width;
    double visible_width = w - overflow;

    /*
     * Width of the falloff gradient. Depends on how much of the block is hidden
     */
    double fsize = std::max(5.0, std::min(std::abs(overflow), 30.0));
    m_log.trace("renderer: Drawing falloff (pos=%g, size=%g, overflow=%g)", visible_width - fsize, fsize, overflow);
    m_context->save();
    *m_context << cairo::translate{(double)m_rect.x, (double)m_rect.y};
    *m_context << cairo::abspos{0.0, 0.0};
    *m_context << cairo::rect{x + visible_width - fsize, y, fsize, h};
    m_context->clip(true);
    *m_context << cairo::linear_gradient{
        x + visible_width - fsize, y, x + visible_width, y, {rgba{0x00000000}, rgba{0xFF000000}}};
    m_context->paint(0.25);
    m_context->restore();
  }
}

/**
 * Flush pixmap contents onto the target window
 */
void renderer::flush() {
  m_log.trace_x("renderer: flush");

  highlight_clickable_areas();

  m_surface->flush();
  // Copy pixmap onto the window
  m_connection.copy_area(m_pixmap, m_window, m_gcontext, 0, 0, 0, 0, m_bar.size.w, m_bar.size.h);
  m_connection.flush();

  if (!m_snapshot_dst.empty()) {
    try {
      m_surface->write_png(m_snapshot_dst);
      m_log.notice("Successfully wrote %s", m_snapshot_dst);
    } catch (const exception& err) {
      m_log.err("Failed to write snapshot (err: %s)", err.what());
    }
    m_snapshot_dst.clear();
  }
}

/**
 * Get x position of block for given alignment
 *
 * The position is relative to m_rect.x (the left side of the bar w/o borders and tray)
 */
double renderer::block_x(alignment a) const {
  switch (a) {
    case alignment::CENTER: {
      // The leftmost x position this block can start at
      double min_pos = block_w(alignment::LEFT);

      if (min_pos != 0) {
        min_pos += BLOCK_GAP;
      }

      double right_width = block_w(alignment::RIGHT);
      /*
       * The rightmost x position this block can end at
       *
       * We can't use block_x(alignment::RIGHT) because that would lead to infinite recursion
       */
      double max_pos = m_rect.width - right_width;

      if (right_width != 0) {
        max_pos -= BLOCK_GAP;
      }

      /*
       * x position of the center of this block
       *
       * With fixed-center this will be the center of the bar unless it is pushed to the left by a large right block
       * Without fixed-center this will be the middle between the end of the left and the start of the right block.
       */
      double base_pos{0.0};

      if (m_fixedcenter) {
        /*
         * This is in the middle of the *bar*. Not just the middle of m_rect because this way we need to account for the
         * tray.
         *
         * The resulting position is relative to the very left of the bar (including border and tray), so we need to
         * compensate for that by subtracting m_rect.x
         */
        base_pos = m_bar.size.w / 2.0 - m_rect.x;

        /*
         * The center block can be moved to the left if the right block is too large
         */
        base_pos = std::min(base_pos, max_pos - block_w(a) / 2.0);
      } else {
        base_pos = (min_pos + max_pos) / 2.0;
      }

      /*
       * The left block always has priority (even with fixed-center = true)
       */
      return std::max(base_pos - block_w(a) / 2.0, min_pos);
    }
    case alignment::RIGHT: {
      /*
       * The block immediately to the left of this block
       *
       * Generally the center block unless it is empty.
       */
      alignment left_barrier = alignment::CENTER;

      if (block_w(alignment::CENTER) == 0) {
        left_barrier = alignment::LEFT;
      }

      // The minimum x position this block can start at
      double min_pos = block_x(left_barrier) + block_w(left_barrier);

      if (block_w(left_barrier) != 0) {
        min_pos += BLOCK_GAP;
      }

      return std::max(m_rect.width - block_w(a), min_pos);
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
  return m_blocks.at(a).width;
}

/**
 * Get block height for given alignment
 */
double renderer::block_h(alignment) const {
  return m_rect.height;
}

void renderer::increase_x(double dx) {
  m_blocks[m_align].x += dx;
  /*
   * The width only increases when x becomes larger than the old width.
   */
  m_blocks[m_align].width = std::max(m_blocks[m_align].width, m_blocks[m_align].x);
}

/**
 * Fill background color
 */
void renderer::fill_background() {
  m_context->save();
  *m_context << m_comp_bg;

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
void renderer::fill_overline(rgba color, double x, double w) {
  if (m_bar.overline.size) {
    m_log.trace_x("renderer: overline(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << m_comp_ol;
    *m_context << color;
    *m_context << cairo::rect{x, static_cast<double>(m_rect.y), w, static_cast<double>(m_bar.overline.size)};
    m_context->fill();
    m_context->restore();
  }
}

/**
 * Fill underline color
 */
void renderer::fill_underline(rgba color, double x, double w) {
  if (m_bar.underline.size) {
    m_log.trace_x("renderer: underline(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << m_comp_ul;
    *m_context << color;
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

  // Draw round border corners

  if (m_bar.radius.top_left) {
    cairo::circle_segment borderTL;
    borderTL.radius = m_bar.borders.at(edge::LEFT).size + m_bar.radius.top_left;
    borderTL.x = m_bar.borders.at(edge::LEFT).size + m_bar.radius.top_left;
    borderTL.y = m_bar.borders.at(edge::TOP).size + m_bar.radius.top_left;
    borderTL.angle_from = 180;
    borderTL.angle_to = 270;
    borderTL.w = m_bar.borders.at(edge::LEFT).size;
    (*m_context << borderTL << m_bar.borders.at(edge::LEFT).color).fill();
  }

  if (m_bar.radius.bottom_left) {
    cairo::circle_segment borderBL;
    borderBL.radius = m_bar.borders.at(edge::LEFT).size + m_bar.radius.bottom_left;
    borderBL.x = m_bar.borders.at(edge::LEFT).size + m_bar.radius.bottom_left;
    borderBL.y = m_bar.size.h - (m_bar.borders.at(edge::BOTTOM).size + m_bar.radius.bottom_left);
    borderBL.angle_from = 90;
    borderBL.angle_to = 180;
    borderBL.w = m_bar.borders.at(edge::LEFT).size;
    (*m_context << borderBL << m_bar.borders.at(edge::LEFT).color).fill();
  }

  if (m_bar.radius.top_right) {
    cairo::circle_segment borderTR;
    borderTR.radius = m_bar.borders.at(edge::RIGHT).size + m_bar.radius.top_right;
    borderTR.x = m_bar.size.w - (m_bar.borders.at(edge::RIGHT).size + m_bar.radius.top_right);
    borderTR.y = m_bar.borders.at(edge::TOP).size + m_bar.radius.top_right;
    borderTR.angle_from = -90;
    borderTR.angle_to = 0;
    borderTR.w = m_bar.borders.at(edge::RIGHT).size;
    (*m_context << borderTR << m_bar.borders.at(edge::RIGHT).color).fill();
  }

  if (m_bar.radius.bottom_right) {
    cairo::circle_segment borderBR;
    borderBR.radius = m_bar.borders.at(edge::RIGHT).size + m_bar.radius.bottom_right;
    borderBR.x = m_bar.size.w - (m_bar.borders.at(edge::RIGHT).size + m_bar.radius.bottom_right);
    borderBR.y = m_bar.size.h - (m_bar.borders.at(edge::BOTTOM).size + m_bar.radius.bottom_right);
    borderBR.angle_from = 0;
    borderBR.angle_to = 90;
    borderBR.w = m_bar.borders.at(edge::RIGHT).size;
    (*m_context << borderBR << m_bar.borders.at(edge::RIGHT).color).fill();
  }

  // Draw straight horizontal / vertical borders

  if (m_bar.borders.at(edge::TOP).size) {
    cairo::rect top{0.0, 0.0, 0.0, 0.0};
    top.x += m_bar.borders.at(edge::LEFT).size;
    top.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
    top.h += m_bar.borders.at(edge::TOP).size;

    if (m_bar.radius.top_left) {
      top.x += m_bar.radius.top_left;
      top.w -= m_bar.radius.top_left;
    }

    if (m_bar.radius.top_right) {
      top.w -= m_bar.radius.top_right;
    }

    m_log.trace_x("renderer: border T(%.0f, #%08x)", top.h, m_bar.borders.at(edge::TOP).color);
    (*m_context << top << m_bar.borders.at(edge::TOP).color).fill();
  }

  if (m_bar.borders.at(edge::BOTTOM).size) {
    cairo::rect bottom{0.0, 0.0, 0.0, 0.0};
    bottom.x += m_bar.borders.at(edge::LEFT).size;
    bottom.y += m_bar.size.h - m_bar.borders.at(edge::BOTTOM).size;
    bottom.w += m_bar.size.w - m_bar.borders.at(edge::LEFT).size - m_bar.borders.at(edge::RIGHT).size;
    bottom.h += m_bar.borders.at(edge::BOTTOM).size;

    if (m_bar.radius.bottom_left) {
      bottom.x += m_bar.radius.bottom_left;
      bottom.w -= m_bar.radius.bottom_left;
    }

    if (m_bar.radius.bottom_right) {
      bottom.w -= m_bar.radius.bottom_right;
    }

    m_log.trace_x("renderer: border B(%.0f, #%08x)", bottom.h, m_bar.borders.at(edge::BOTTOM).color);
    (*m_context << bottom << m_bar.borders.at(edge::BOTTOM).color).fill();
  }

  if (m_bar.borders.at(edge::LEFT).size) {
    cairo::rect left{0.0, 0.0, 0.0, 0.0};
    left.w += m_bar.borders.at(edge::LEFT).size;
    left.h += m_bar.size.h;

    if (m_bar.radius.top_left) {
      left.y += m_bar.radius.top_left + m_bar.borders.at(edge::TOP).size;
      left.h -= m_bar.radius.top_left + m_bar.borders.at(edge::TOP).size;
    }

    if (m_bar.radius.bottom_left) {
      left.h -= m_bar.radius.bottom_left + m_bar.borders.at(edge::BOTTOM).size;
    }

    m_log.trace_x("renderer: border L(%.0f, #%08x)", left.w, m_bar.borders.at(edge::LEFT).color);
    (*m_context << left << m_bar.borders.at(edge::LEFT).color).fill();
  }

  if (m_bar.borders.at(edge::RIGHT).size) {
    cairo::rect right{0.0, 0.0, 0.0, 0.0};
    right.x += m_bar.size.w - m_bar.borders.at(edge::RIGHT).size;
    right.w += m_bar.borders.at(edge::RIGHT).size;
    right.h += m_bar.size.h;

    if (m_bar.radius.top_right) {
      right.y += m_bar.radius.top_right + m_bar.borders.at(edge::TOP).size;
      right.h -= m_bar.radius.top_right + m_bar.borders.at(edge::TOP).size;
    }

    if (m_bar.radius.bottom_right) {
      right.h -= m_bar.radius.bottom_right + m_bar.borders.at(edge::BOTTOM).size;
    }

    m_log.trace_x("renderer: border R(%.0f, #%08x)", right.w, m_bar.borders.at(edge::RIGHT).color);
    (*m_context << right << m_bar.borders.at(edge::RIGHT).color).fill();
  }

  m_context->restore();
}

/**
 * Draw text contents
 */
void renderer::render_text(const tags::context& ctxt, const string&& contents) {
  assert(ctxt.get_alignment() != alignment::NONE && ctxt.get_alignment() == m_align);
  m_log.trace_x("renderer: text(%s)", contents.c_str());

  cairo::abspos origin{};
  origin.x = m_rect.x + m_blocks[m_align].x;
  origin.y = m_rect.y + m_rect.height / 2.0;

  double x_old = m_blocks[m_align].x;
  /*
   * This variable is increased by the text renderer
   */
  double x_new = x_old;

  cairo::textblock block{};
  block.align = m_align;
  block.contents = contents;
  block.font = ctxt.get_font();
  block.x_advance = &x_new;
  block.y_advance = &m_blocks[m_align].y;
  block.bg_rect = cairo::rect{0.0, 0.0, 0.0, 0.0};

  rgba bg = ctxt.get_bg();

  // Only draw text background if the color differs from
  // the background color of the bar itself
  // Note: this means that if the user explicitly set text
  // background color equal to background-0 it will be ignored
  if (bg != m_bar.background) {
    block.bg = bg;
    block.bg_operator = m_comp_bg;
    block.bg_rect.x = m_rect.x;
    block.bg_rect.y = m_rect.y;
    block.bg_rect.h = m_rect.height;
  }

  m_context->save();
  *m_context << origin;
  *m_context << m_comp_fg;
  *m_context << ctxt.get_fg();
  *m_context << block;
  m_context->restore();

  double dx = x_new - x_old;
  increase_x(dx);

  if (dx > 0.0) {
    if (ctxt.has_underline()) {
      fill_underline(ctxt.get_ul(), origin.x, dx);
    }

    if (ctxt.has_overline()) {
      fill_overline(ctxt.get_ol(), origin.x, dx);
    }
  }
}

void renderer::draw_offset(const tags::context& ctxt, rgba color, double x, double w) {
  if (w <= 0) {
    return;
  }

  if (color != m_bar.background) {
    m_log.trace_x("renderer: offset(x=%f, w=%f)", x, w);
    m_context->save();
    *m_context << m_comp_bg;
    *m_context << color;
    *m_context << cairo::rect{
        m_rect.x + x, static_cast<double>(m_rect.y), w, static_cast<double>(m_rect.y + m_rect.height)};
    m_context->fill();
    m_context->restore();
  }

  if (ctxt.has_underline()) {
    fill_underline(ctxt.get_ul(), x, w);
  }

  if (ctxt.has_overline()) {
    fill_overline(ctxt.get_ol(), x, w);
  }
}

void renderer::render_offset(const tags::context& ctxt, const extent_val offset) {
  assert(ctxt.get_alignment() != alignment::NONE && ctxt.get_alignment() == m_align);
  m_log.trace_x("renderer: offset_pixel(%f)", offset);

  int offset_width = units_utils::extent_to_pixel(offset, m_bar.dpi_x);
  rgba bg = ctxt.get_bg();
  draw_offset(ctxt, bg, m_blocks[m_align].x, offset_width);
  increase_x(offset_width);
}

void renderer::change_alignment(const tags::context& ctxt) {
  auto align = ctxt.get_alignment();
  assert(align != alignment::NONE);
  if (align != m_align) {
    m_log.trace_x("renderer: change_alignment(%i)", static_cast<int>(align));

    if (m_align != alignment::NONE) {
      m_log.trace_x("renderer: pop(%i)", static_cast<int>(m_align));
      m_context->pop(&m_blocks[m_align].pattern);
    }

    m_align = align;
    m_blocks[m_align].x = 0.0;
    m_blocks[m_align].y = 0.0;
    m_blocks[m_align].width = 0.;
    m_context->push();
    m_log.trace_x("renderer: push(%i)", static_cast<int>(m_align));

    fill_background();
  }
}

double renderer::get_x(const tags::context& ctxt) const {
  assert(ctxt.get_alignment() != alignment::NONE && ctxt.get_alignment() == m_align);
  return m_blocks.at(ctxt.get_alignment()).x;
}

double renderer::get_alignment_start(const alignment align) const {
  return block_x(align) + m_rect.x;
}

/**
 * Colorize the bounding box of created action blocks
 */
void renderer::highlight_clickable_areas() {
#ifdef DEBUG_HINTS
  map<alignment, int> hint_num{};
  for (auto&& action : m_action_ctxt.get_blocks()) {
    int n = hint_num.find(action.align)->second++;
    double x = action.start_x;
    double y = m_rect.y;
    double w = action.width();
    double h = m_rect.height;

    m_context->save();
    *m_context << CAIRO_OPERATOR_DIFFERENCE << (n % 2 ? rgba{0xFF00FF00} : rgba{0xFFFF0000});
    *m_context << cairo::rect{x, y, w, h};
    m_context->fill();
    m_context->restore();
  }
  m_surface->flush();
#endif
}

bool renderer::on(const signals::ui::request_snapshot& evt) {
  m_snapshot_dst = evt.cast();
  return true;
}

void renderer::apply_tray_position(const tags::context& context) {
  auto [alignment, pos] = context.get_relative_tray_position();
  if (alignment != alignment::NONE) {
    int absolute_x = static_cast<int>(block_x(alignment) + pos);
    m_sig.emit(signals::ui_tray::tray_pos_change{absolute_x});
  }
}

POLYBAR_NS_END

#include "components/renderer.hpp"
#include "cairo/context.hpp"
#include "cairo/font.hpp"
#include "cairo/surface.hpp"
#include "cairo/types.hpp"
#include "cairo/utils.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "errors.hpp"
#include "events/signal.hpp"
#include "events/signal_receiver.hpp"
#include "utils/color.hpp"
#include "utils/factory.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"
#include "x11/atoms.hpp"
#include "x11/connection.hpp"
#include "x11/draw.hpp"
#include "x11/extensions/all.hpp"
#include "x11/generic.hpp"
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

  m_log.trace("renderer: Allocate window pixmap");
  {
    m_pixmap = m_connection.generate_id();
    m_connection.create_pixmap(m_depth, m_pixmap, m_window, m_bar.size.w, m_bar.size.h);
  }

  m_log.trace("renderer: Allocate graphic context");
  {
    uint32_t mask{0};
    uint32_t value_list[32]{0};
    xcb_params_gc_t params{};
    XCB_AUX_ADD_PARAM(&mask, &params, foreground, m_bar.foreground);
    XCB_AUX_ADD_PARAM(&mask, &params, graphics_exposures, 0);
    connection::pack_values(mask, &params, value_list);
    m_gcontext = m_connection.generate_id();
    m_connection.create_gc(m_gcontext, m_pixmap, mask, value_list);
  }

  m_log.trace("renderer: Allocate cairo components");
  {
    m_surface = make_unique<cairo::xcb_surface>(m_connection, m_pixmap, m_visual, m_bar.size.w, m_bar.size.h);
    m_context = make_unique<cairo::context>(*m_surface.get(), m_log);
  }

  m_log.trace("renderer: Load fonts");
  {
    auto fonts = m_conf.get_list<string>(m_conf.section(), "font", {});
    if (fonts.empty()) {
      m_log.warn("No fonts specified, using fallback font \"fixed\"");
      fonts.emplace_back("fixed");
    }

    auto fonts_loaded = false;
    for (const auto& f : fonts) {
      vector<string> fd{string_util::split(f, ';')};
      auto font = cairo::make_font(*m_context, string(fd[0]), fd.size() > 1 ? std::atoi(fd[1].c_str()) : 0);
      m_log.info("Loaded font \"%s\" (name=%s, file=%s)", fd[0], font->name(), font->file());
      *m_context << move(font);
      fonts_loaded = true;
    }

    if (!fonts_loaded) {
      throw application_error("Unable to load fonts");
    }
  }

  m_compositing_background =
      cairo::utils::str2operator(m_conf.get("settings", "compositing-background", ""s), CAIRO_OPERATOR_SOURCE);
  m_compositing_foreground =
      cairo::utils::str2operator(m_conf.get("settings", "compositing-foreground", ""s), CAIRO_OPERATOR_OVER);
  m_compositing_overline =
      cairo::utils::str2operator(m_conf.get("settings", "compositing-overline", ""s), CAIRO_OPERATOR_OVER);
  m_compositing_underline =
      cairo::utils::str2operator(m_conf.get("settings", "compositing-underline", ""s), CAIRO_OPERATOR_OVER);
  m_compositing_borders =
      cairo::utils::str2operator(m_conf.get("settings", "compositing-border", ""s), CAIRO_OPERATOR_SOURCE);
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
  m_attributes.reset();
  m_alignment = alignment::NONE;
  m_rect = m_bar.inner_area();
  m_x = 0.0;

  // Reset colors
  m_color_background = 0;
  m_color_foreground = m_bar.foreground;
  m_color_underline = m_bar.underline.color;
  m_color_overline = m_bar.overline.color;

  // Clear canvas
  m_context->save();
  *m_context << CAIRO_OPERATOR_SOURCE;
  *m_context << rgba{0.0, 0.0, 0.0, 0.0};
  m_context->paint();
  m_context->restore();

  m_context->save();

  fill_background();
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

  highlight_clickable_areas();

  m_context->restore();
  m_surface->flush();

  flush();
}

/**
 * Flush pixmap contents onto the target window
 */
void renderer::flush() {
  m_log.trace_x("renderer: flush");

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

  m_log.trace_x("renderer: copy pixmap (geom=%dx%d+%d+%d)", m_rect.width, m_rect.height, m_rect.x, m_rect.y);
  m_connection.copy_area(m_pixmap, m_window, m_gcontext, 0, 0, 0, 0, m_bar.size.w, m_bar.size.h);
  m_connection.flush();
}

/**
 * Reserve space at given edge
 */
void renderer::reserve_space(edge side, uint16_t w) {
  m_log.trace_x("renderer: reserve_space(%i, %i)", static_cast<uint8_t>(side), w);

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
  *m_context << m_compositing_background;

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
  if (m_bar.overline.size && m_attributes.test(static_cast<uint8_t>(attribute::OVERLINE))) {
    m_log.trace_x("renderer: overline(x=%i, w=%i)", x, w);
    m_context->save();
    *m_context << m_compositing_overline;
    *m_context << m_color_overline;
    *m_context << cairo::rect{x, static_cast<double>(m_rect.y), w, static_cast<double>(m_bar.overline.size)};
    m_context->fill();
    m_context->restore();
  }
}

/**
 * Fill underline color
 */
void renderer::fill_underline(double x, double w) {
  if (m_bar.underline.size && m_attributes.test(static_cast<uint8_t>(attribute::UNDERLINE))) {
    m_log.trace_x("renderer: underline(x=%i, w=%i)", x, w);
    m_context->save();
    *m_context << m_compositing_underline;
    *m_context << m_color_underline;
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
  *m_context << m_compositing_borders;

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

  cairo_text_extents_t extents;
  cairo_text_extents(*m_context, contents.c_str(), &extents);

  if (!extents.width) {
    return;
  }

  cairo::abspos origin{static_cast<double>(m_rect.x), static_cast<double>(m_rect.y)};

  if (m_alignment == alignment::CENTER) {
    origin.x += m_rect.width / 2.0 - extents.width / 2.0;
    adjust_clickable_areas(extents.width / 2.0);
  } else if (m_alignment == alignment::RIGHT) {
    origin.x += m_rect.width - extents.width;
    adjust_clickable_areas(extents.width);
  } else {
    origin.x += m_x;
  }

  // if (m_color_background && m_color_background != m_bar.background) {
  //   m_context->save();
  //   *m_context << m_color_background;
  //   *m_context << m_compositing_background;
  //   *m_context << cairo::rect{origin.x, origin.y, extents.width, static_cast<double>(m_rect.height)};
  //   m_context->fill();
  //   m_context->restore();
  // }

  origin.y += m_rect.height / 2.0;

  m_context->save();
  *m_context << origin;
  *m_context << m_compositing_foreground;
  *m_context << m_color_foreground;
  *m_context << cairo::textblock{contents, m_fontindex};
  m_context->position(&m_x, &m_y);
  m_context->restore();

  // if (m_alignment == alignment::CENTER) {
  //   m_x += extents.width / 2.0;
  // } else {
  //   m_x += extents.width;
  // }

  // fill_underline(origin.x, m_x - origin.x);
  // fill_overline(origin.x, m_x - origin.x);
}

/**
 * Move clickable areas position by given delta
 */
void renderer::adjust_clickable_areas(double delta) {
  for (auto&& action : m_actions) {
    if (!action.active && action.align == m_alignment) {
      action.start_x -= delta;
      action.end_x -= delta;
    }
  }
}

/**
 * Draw boxes at the location of each created action block
 */
void renderer::highlight_clickable_areas() {
#ifdef DEBUG_HINTS
  map<alignment, int> hint_num{};
  for (auto&& action : m_actions) {
    if (!action.active) {
      uint8_t n = hint_num.find(action.align)->second++;
      double x = action.start_x + n * DEBUG_HINTS_OFFSET_X;
      double y = m_bar.pos.y + m_rect.y + n * DEBUG_HINTS_OFFSET_Y;
      double w = action.width();
      double h = m_rect.height;

      m_context->save();
      *m_context << CAIRO_OPERATOR_OVERLAY << (n % 2 ? 0x55FF0000 : 0x5500FF00);
      *m_context << cairo::rect{x, y, w, h};
      m_context->fill();
      m_context->restore();
    }
  }
#endif
}

bool renderer::on(const signals::parser::change_background& evt) {
  const uint32_t color{evt.cast()};
  if (color != m_color_background) {
    m_color_background = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_foreground& evt) {
  const uint32_t color{evt.cast()};
  if (color != m_color_foreground) {
    m_color_foreground = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_underline& evt) {
  const uint32_t color{evt.cast()};
  if (color != m_color_underline) {
    m_color_underline = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_overline& evt) {
  const uint32_t color{evt.cast()};
  if (color != m_color_overline) {
    m_color_overline = color;
  }
  return true;
}

bool renderer::on(const signals::parser::change_font& evt) {
  m_fontindex = evt.cast();
  return true;
}

bool renderer::on(const signals::parser::change_alignment& evt) {
  auto align = static_cast<const alignment&>(evt.cast());
  if (align != m_alignment) {
    m_alignment = align;
    m_x = 0.0;
  }
  return true;
}

bool renderer::on(const signals::parser::offset_pixel& evt) {
  m_x += evt.cast();
  return true;
}

bool renderer::on(const signals::parser::attribute_set& evt) {
  m_attributes.set(static_cast<uint8_t>(evt.cast()), true);
  return true;
}

bool renderer::on(const signals::parser::attribute_unset& evt) {
  m_attributes.set(static_cast<uint8_t>(evt.cast()), false);
  return true;
}

bool renderer::on(const signals::parser::attribute_toggle& evt) {
  m_attributes.flip(static_cast<uint8_t>(evt.cast()));
  return true;
}

bool renderer::on(const signals::parser::action_begin& evt) {
  (void) evt;
  // auto a = evt.cast();
  // action_block action{};
  // action.button = a.button == mousebtn::NONE ? mousebtn::LEFT : a.button;
  // action.align = m_alignment;
  // action.start_x = m_x;
  // action.command = string_util::replace_all(a.command, ":", "\\:");
  // action.active = true;
  // m_actions.emplace_back(action);
  return true;
}

bool renderer::on(const signals::parser::action_end& evt) {
  (void) evt;
  // auto btn = evt.cast();
  // int16_t clickable_width = 0;
  // for (auto action = m_actions.rbegin(); action != m_actions.rend(); action++) {
  //   if (action->active && action->align == m_alignment && action->button == btn) {
  //     switch (action->align) {
  //       case alignment::NONE:
  //         break;
  //       case alignment::LEFT:
  //         action->end_x = m_x;
  //         break;
  //       case alignment::CENTER:
  //         clickable_width = m_x - action->start_x;
  //         action->start_x = m_rect.width / 2 - clickable_width / 2 + action->start_x / 2;
  //         action->end_x = action->start_x + clickable_width;
  //         break;
  //       case alignment::RIGHT:
  //         action->start_x = m_rect.width - m_x + action->start_x;
  //         action->end_x = m_rect.width;
  //         break;
  //     }
  //     action->start_x += m_rect.x;
  //     action->end_x += m_rect.x;
  //     action->active = false;
  //   }
  // }
  return true;
}

bool renderer::on(const signals::parser::text& evt) {
  auto text = evt.cast();
  draw_text(text);
  return true;
}

POLYBAR_NS_END

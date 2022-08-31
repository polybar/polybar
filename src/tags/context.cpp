#include "tags/context.hpp"

POLYBAR_NS

namespace tags {
  static rgba get_color(color_value c, rgba fallback) {
    if (c.type == color_type::RESET) {
      return fallback;
    } else {
      return c.val;
    }
  }

  context::context(const bar_settings& settings) : m_settings(settings) {
    reset();
  }

  void context::reset() {
    apply_reset();
    m_align = alignment::NONE;
  }

  void context::apply_bg(color_value c) {
    m_bg = get_color(c, m_settings.background);
  }

  void context::apply_fg(color_value c) {
    m_fg = get_color(c, m_settings.foreground);
  }

  void context::apply_ol(color_value c) {
    m_ol = get_color(c, m_settings.overline.color);
  }

  void context::apply_ul(color_value c) {
    m_ul = get_color(c, m_settings.underline.color);
  }

  void context::apply_font(int font) {
    m_font = std::max(font, 0);
  }

  void context::apply_reverse() {
    std::swap(m_bg, m_fg);
  }

  void context::apply_alignment(alignment align) {
    m_align = align;
  }

  void context::apply_attr(attr_activation act, attribute attr) {
    if (attr == attribute::NONE) {
      return;
    }

    bool& current = attr == attribute::OVERLINE ? m_attr_overline : m_attr_underline;

    switch (act) {
      case attr_activation::ON:
        current = true;
        break;
      case attr_activation::OFF:
        current = false;
        break;
      case attr_activation::TOGGLE:
        current = !current;
        break;
      default:
        break;
    }
  }

  void context::store_tray_position(int x_pos) {
    m_relative_tray_position = std::make_pair(get_alignment(), x_pos);
  }

  void context::apply_reset() {
    m_bg = m_settings.background;
    m_fg = m_settings.foreground;
    m_ul = m_settings.underline.color;
    m_ol = m_settings.overline.color;
    m_font = 0;
    m_attr_overline = false;
    m_attr_underline = false;
  }

  rgba context::get_bg() const {
    return m_bg;
  }

  rgba context::get_fg() const {
    return m_fg;
  }

  rgba context::get_ol() const {
    return m_ol;
  }

  rgba context::get_ul() const {
    return m_ul;
  }

  int context::get_font() const {
    return m_font;
  }

  bool context::has_overline() const {
    return m_attr_overline;
  }

  bool context::has_underline() const {
    return m_attr_underline;
  }

  alignment context::get_alignment() const {
    return m_align;
  }

  std::pair<alignment, int> context::get_relative_tray_position() const {
    return m_relative_tray_position;
  }
}  // namespace tags

POLYBAR_NS_END

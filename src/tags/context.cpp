#include "tags/context.hpp"

#include <cassert>

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

  void action_context::reset() {
    m_action_blocks.clear();
  }

  action_t action_context::action_open(mousebtn btn, const string&& cmd, alignment align) {
    action_t id = m_action_blocks.size();
    m_action_blocks.emplace_back(std::move(cmd), btn, align, true);
    return id;
  }

  std::pair<action_t, mousebtn> action_context::action_close(mousebtn btn, alignment align) {
    for (auto it = m_action_blocks.rbegin(); it != m_action_blocks.rend(); it++) {
      if (it->is_open && it->align == align && (btn == mousebtn::NONE || it->button == btn)) {
        it->is_open = false;

        // Converts a reverse iterator into an index
        return {std::distance(m_action_blocks.begin(), it.base()) - 1, it->button};
      }
    }

    return {NO_ACTION, mousebtn::NONE};
  }

  void action_context::set_start(action_t id, double x) {
    m_action_blocks[id].start_x = x;
  }

  void action_context::set_end(action_t id, double x) {
    m_action_blocks[id].end_x = x;
  }

  std::map<mousebtn, tags::action_t> action_context::get_actions(int x) const {
    std::map<mousebtn, tags::action_t> buttons;

    for (int i = static_cast<int>(mousebtn::NONE); i < static_cast<int>(mousebtn::BTN_COUNT); i++) {
      buttons[static_cast<mousebtn>(i)] = tags::NO_ACTION;
    }

    for (action_t id = 0; (unsigned)id < m_action_blocks.size(); id++) {
      auto action = m_action_blocks[id];
      mousebtn btn = action.button;

      // Higher IDs are higher in the action stack.
      if (id > buttons[btn] && action.test(x)) {
        buttons[action.button] = id;
      }
    }

    return buttons;
  }

  action_t action_context::has_action(mousebtn btn, int x) const {
    // TODO optimize
    return get_actions(x)[btn];
  }

  string action_context::get_action(action_t id) const {
    assert(id >= 0 && (unsigned)id < num_actions());

    return m_action_blocks[id].cmd;
  }

  bool action_context::has_double_click() const {
    for (auto&& a : m_action_blocks) {
      if (a.button == mousebtn::DOUBLE_LEFT || a.button == mousebtn::DOUBLE_MIDDLE ||
          a.button == mousebtn::DOUBLE_RIGHT) {
        return true;
      }
    }

    return false;
  }

  size_t action_context::num_actions() const {
    return m_action_blocks.size();
  }

  std::vector<action_block>& action_context::get_blocks() {
    return m_action_blocks;
  }

}  // namespace tags

POLYBAR_NS_END

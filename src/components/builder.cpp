#include "components/builder.hpp"

#include <utility>

#include "drawtypes/label.hpp"
#include "utils/actions.hpp"
#include "utils/color.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/unit.hpp"
POLYBAR_NS

using namespace tags;

builder::builder(const bar_settings& bar) : m_bar(bar) {
  reset();
}

void builder::reset() {
  /* Add all values as keys so that we never have to check if a key exists in
   * the map
   */
  m_tags.clear();
  m_tags[syntaxtag::A] = 0;
  m_tags[syntaxtag::B] = 0;
  m_tags[syntaxtag::F] = 0;
  m_tags[syntaxtag::T] = 0;
  m_tags[syntaxtag::R] = 0;
  m_tags[syntaxtag::o] = 0;
  m_tags[syntaxtag::u] = 0;
  m_tags[syntaxtag::P] = 0;

  m_colors.clear();
  m_colors[syntaxtag::B] = string();
  m_colors[syntaxtag::F] = string();
  m_colors[syntaxtag::o] = string();
  m_colors[syntaxtag::u] = string();

  m_attrs.clear();
  m_attrs[attribute::NONE] = false;
  m_attrs[attribute::UNDERLINE] = false;
  m_attrs[attribute::OVERLINE] = false;

  m_output.clear();
  m_fontindex = 1;
}

/**
 * Flush contents of the builder and return built string
 *
 * This will also close any unclosed tags
 */
string builder::flush() {
  if (m_tags[syntaxtag::B]) {
    background_close();
  }
  if (m_tags[syntaxtag::F]) {
    color_close();
  }
  if (m_tags[syntaxtag::T]) {
    font_close();
  }
  if (m_tags[syntaxtag::o]) {
    overline_color_close();
  }
  if (m_tags[syntaxtag::u]) {
    underline_color_close();
  }
  if (m_attrs[attribute::UNDERLINE]) {
    underline_close();
  }
  if (m_attrs[attribute::OVERLINE]) {
    overline_close();
  }

  while (m_tags[syntaxtag::A]) {
    action_close();
  }

  string output{m_output};

  reset();

  return output;
}

/**
 * Insert raw text string
 */
void builder::append(string text) {
  m_output.reserve(text.size());
  m_output += move(text);
}

/**
 * Insert text node
 *
 * This will also parse raw syntax tags
 */
void builder::node(string str) {
  if (str.empty()) {
    return;
  }

  append(move(str));
}

/**
 * Insert text node with specific font index
 *
 * \see builder::node
 */
void builder::node(string str, int font_index) {
  font(font_index);
  node(move(str));
  font_close();
}

/**
 * Insert tags for given label
 */
void builder::node(const label_t& label) {
  if (!label || !*label) {
    return;
  }

  auto text = label->get();

  if (label->m_margin.left.value > 0) {
    spacing(label->m_margin.left);
  }

  if (label->m_overline.has_color()) {
    overline(label->m_overline);
  }
  if (label->m_underline.has_color()) {
    underline(label->m_underline);
  }

  if (label->m_background.has_color()) {
    background(label->m_background);
  }
  if (label->m_foreground.has_color()) {
    color(label->m_foreground);
  }

  if (label->m_padding.left.value > 0) {
    spacing(label->m_padding.left);
  }

  node(text, label->m_font);

  if (label->m_padding.right.value > 0) {
    spacing(label->m_padding.right);
  }

  if (label->m_background.has_color()) {
    background_close();
  }
  if (label->m_foreground.has_color()) {
    color_close();
  }

  if (label->m_underline.has_color()) {
    underline_close();
  }
  if (label->m_overline.has_color()) {
    overline_close();
  }

  if (label->m_margin.right.value > 0) {
    spacing(label->m_margin.right);
  }
}

/**
 * Repeat text string n times
 */
void builder::node_repeat(const string& str, size_t n) {
  string text;
  text.reserve(str.size() * n);
  while (n--) {
    text += str;
  }
  node(text);
}

/**
 * Repeat label contents n times
 */
void builder::node_repeat(const label_t& label, size_t n) {
  string text;
  string label_text{label->get()};
  text.reserve(label_text.size() * n);
  while (n--) {
    text += label_text;
  }
  label_t tmp{new label_t::element_type{text}};
  tmp->replace_defined_values(label);
  node(tmp);
}

/**
 * Insert tag that will offset the contents by the given extent
 */
void builder::offset(extent_val extent) {
  if (extent.value == 0) {
    return;
  }
  tag_open(syntaxtag::O, unit_utils::extent_to_string(extent));
}

/**
 * Insert spacing
 */
void builder::spacing(spacing_val size) {
  if (size.value > 0.) {
    m_output += get_spacing_format_string(size);
  } else {
    // TODO remove once the deprecated spacing key in the bar section is removed
    spacing();
  }
}
void builder::spacing() {
  if (m_bar.spacing.value > 0) {
    spacing(m_bar.spacing);
  }
}

/**
 * Remove trailing space
 */
void builder::remove_trailing_space(size_t len) {
  if (len == 0_z || len > m_output.size()) {
    return;
  } else if (m_output.substr(m_output.size() - len) == string(len, ' ')) {
    m_output.erase(m_output.size() - len);
  }
}

/**
 * Insert tag to alter the current font index
 */
void builder::font(int index) {
  if (index == 0) {
    return;
  }
  m_fontindex = index;
  tag_open(syntaxtag::T, to_string(index));
}

/**
 * Insert tag to reset the font index
 */
void builder::font_close() {
  m_fontindex = 1;
  tag_close(syntaxtag::T);
}

/**
 * Insert tag to alter the current background color
 */
void builder::background(rgba color) {
  color = color.try_apply_alpha_to(m_bar.background);

  auto hex = color_util::simplify_hex(color);
  m_colors[syntaxtag::B] = hex;
  tag_open(syntaxtag::B, hex);
}

/**
 * Insert tag to reset the background color
 */
void builder::background_close() {
  m_colors[syntaxtag::B].clear();
  tag_close(syntaxtag::B);
}

/**
 * Insert tag to alter the current foreground color
 */
void builder::color(rgba color) {
  color = color.try_apply_alpha_to(m_bar.foreground);

  auto hex = color_util::simplify_hex(color);
  m_colors[syntaxtag::F] = hex;
  tag_open(syntaxtag::F, hex);
}

/**
 * Insert tag to reset the foreground color
 */
void builder::color_close() {
  m_colors[syntaxtag::F].clear();
  tag_close(syntaxtag::F);
}

/**
 * Insert tag to alter the current overline/underline color
 */
void builder::line_color(const rgba& color) {
  overline_color(color);
  underline_color(color);
}

/**
 * Close overline/underline color tag
 */
void builder::line_color_close() {
  overline_color_close();
  underline_color_close();
}

/**
 * Insert tag to alter the current overline color
 */
void builder::overline_color(rgba color) {
  auto hex = color_util::simplify_hex(color);
  m_colors[syntaxtag::o] = hex;
  tag_open(syntaxtag::o, hex);
  tag_open(attribute::OVERLINE);
}

/**
 * Close underline color tag
 */
void builder::overline_color_close() {
  m_colors[syntaxtag::o].clear();
  tag_close(syntaxtag::o);
}

/**
 * Insert tag to alter the current underline color
 */
void builder::underline_color(rgba color) {
  auto hex = color_util::simplify_hex(color);
  m_colors[syntaxtag::u] = hex;
  tag_open(syntaxtag::u, hex);
  tag_open(attribute::UNDERLINE);
}

/**
 * Close underline color tag
 */
void builder::underline_color_close() {
  tag_close(syntaxtag::u);
  m_colors[syntaxtag::u].clear();
}

/**
 * Insert tag to enable the overline attribute
 */
void builder::overline(const rgba& color) {
  if (color.has_color()) {
    overline_color(color);
  } else {
    tag_open(attribute::OVERLINE);
  }
}

/**
 * Close overline attribute tag
 */
void builder::overline_close() {
  tag_close(attribute::OVERLINE);
}

/**
 * Insert tag to enable the underline attribute
 */
void builder::underline(const rgba& color) {
  if (color.has_color()) {
    underline_color(color);
  } else {
    tag_open(attribute::UNDERLINE);
  }
}

/**
 * Close underline attribute tag
 */
void builder::underline_close() {
  tag_close(attribute::UNDERLINE);
}

/**
 * Add a polybar control tag
 */
void builder::control(controltag tag) {
  string str;
  switch (tag) {
    case controltag::R:
      str = "R";
      break;
    default:
      break;
  }

  if (!str.empty()) {
    tag_open(syntaxtag::P, str);
  }
}

/**
 * Open action tag with the given action string
 *
 * The action string is escaped, if needed.
 */
void builder::action(mousebtn index, string action) {
  if (!action.empty()) {
    action = string_util::replace_all(action, ":", "\\:");
    tag_open(syntaxtag::A, to_string(static_cast<int>(index)) + ":" + action + ":");
  }
}

/**
 * Open action tag for the action of the given module
 */
void builder::action(mousebtn btn, const modules::module_interface& module, string action_name, string data) {
  action(btn, actions_util::get_action_string(module, action_name, data));
}

/**
 * Wrap label in action tag
 */
void builder::action(mousebtn index, string action_name, const label_t& label) {
  if (label && *label) {
    action(index, action_name);
    node(label);
    tag_close(syntaxtag::A);
  }
}

/**
 * Wrap label in module action tag
 */
void builder::action(
    mousebtn btn, const modules::module_interface& module, string action_name, string data, const label_t& label) {
  action(btn, actions_util::get_action_string(module, action_name, data), label);
}

/**
 * Close command tag
 */
void builder::action_close() {
  tag_close(syntaxtag::A);
}

/**
 * Insert directive to change value of given tag
 */
void builder::tag_open(syntaxtag tag, const string& value) {
  m_tags[tag]++;

  switch (tag) {
    case syntaxtag::A:
      append("%{A" + value + "}");
      break;
    case syntaxtag::F:
      append("%{F" + value + "}");
      break;
    case syntaxtag::B:
      append("%{B" + value + "}");
      break;
    case syntaxtag::T:
      append("%{T" + value + "}");
      break;
    case syntaxtag::u:
      append("%{u" + value + "}");
      break;
    case syntaxtag::o:
      append("%{o" + value + "}");
      break;
    case syntaxtag::R:
      append("%{R}");
      break;
    case syntaxtag::O:
      append("%{O" + value + "}");
      break;
    case syntaxtag::P:
      append("%{P" + value + "}");
      break;
    case syntaxtag::l:
      append("%{l}");
      break;
    case syntaxtag::c:
      append("%{c}");
      break;
    case syntaxtag::r:
      append("%{r}");
      break;
  }
}

/**
 * Insert directive to use given attribute unless already set
 */
void builder::tag_open(attribute attr) {
  if (m_attrs[attr]) {
    return;
  }

  m_attrs[attr] = true;

  switch (attr) {
    case attribute::NONE:
      break;
    case attribute::UNDERLINE:
      append("%{+u}");
      break;
    case attribute::OVERLINE:
      append("%{+o}");
      break;
  }
}

/**
 * Insert directive to reset given tag if it's open and closable
 */
void builder::tag_close(syntaxtag tag) {
  if (!m_tags[tag]) {
    return;
  }

  m_tags[tag]--;

  switch (tag) {
    case syntaxtag::A:
      append("%{A}");
      break;
    case syntaxtag::F:
      append("%{F-}");
      break;
    case syntaxtag::B:
      append("%{B-}");
      break;
    case syntaxtag::T:
      append("%{T-}");
      break;
    case syntaxtag::u:
      append("%{u-}");
      break;
    case syntaxtag::o:
      append("%{o-}");
      break;
    case syntaxtag::R:
    case syntaxtag::P:
    case syntaxtag::O:
    case syntaxtag::l:
    case syntaxtag::c:
    case syntaxtag::r:
      break;
  }
}

/**
 * Insert directive to remove given attribute if set
 */
void builder::tag_close(attribute attr) {
  if (!m_attrs[attr]) {
    return;
  }

  m_attrs[attr] = false;

  switch (attr) {
    case attribute::NONE:
      break;
    case attribute::UNDERLINE:
      append("%{-u}");
      break;
    case attribute::OVERLINE:
      append("%{-o}");
      break;
  }
}

string builder::get_spacing_format_string(const spacing_val& space) {
  if (space.value == 0) {
    return "";
  }

  string out;
  if (space.type == spacing_type::SPACE) {
    out += string(space.value, ' ');
  } else {
    out += "%{O";

    switch (space.type) {
      case spacing_type::POINT:
        out += to_string(space.value) + "pt";
        break;
      case spacing_type::PIXEL:
        out += to_string(static_cast<int>(space.value)) + "px";
        break;
      default:
        break;
    }

    out += '}';
  }

  return out;
}

POLYBAR_NS_END

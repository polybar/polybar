#include "components/builder.hpp"

#include <utility>

#include "drawtypes/label.hpp"
#include "utils/actions.hpp"
#include "utils/color.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/units.hpp"

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

  m_attrs.clear();
  m_output.clear();
}

/**
 * Flush contents of the builder and return built string
 *
 * This will also close any unclosed tags
 */
string builder::flush() {
  background_close();
  foreground_close();
  font_close();
  overline_color_close();
  underline_color_close();
  underline_close();
  overline_close();

  while (m_tags[syntaxtag::A]) {
    action_close();
  }

  string output{};
  std::swap(m_output, output);

  reset();

  return output;
}

/**
 * Insert raw text string
 */
void builder::append(const string& text) {
  m_output.reserve(text.size());
  m_output += text;
}

/**
 * Insert text node
 *
 * This will also parse raw syntax tags
 */
void builder::node(const string& str) {
  if (str.empty()) {
    return;
  }

  append(str);
}

/**
 * Insert text node with specific font index
 *
 * @see builder::node
 */
void builder::node(const string& str, int font_index) {
  font(font_index);
  node(str);
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

  if (label->m_margin.left) {
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
    foreground(label->m_foreground);
  }

  if (label->m_padding.left) {
    spacing(label->m_padding.left);
  }

  node(text, label->m_font);

  if (label->m_padding.right) {
    spacing(label->m_padding.right);
  }

  if (label->m_background.has_color()) {
    background_close();
  }
  if (label->m_foreground.has_color()) {
    foreground_close();
  }

  if (label->m_underline.has_color()) {
    underline_close();
  }
  if (label->m_overline.has_color()) {
    overline_close();
  }

  if (label->m_margin.right) {
    spacing(label->m_margin.right);
  }
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
  if (!extent) {
    return;
  }
  tag_open(syntaxtag::O, units_utils::extent_to_string(extent));
}

/**
 * Insert spacing
 */
void builder::spacing(spacing_val size) {
  if (!size && m_bar.spacing) {
    // TODO remove once the deprecated spacing key in the bar section is removed
    // The spacing in the bar section acts as a fallback for all spacing value
    size = m_bar.spacing;
  }

  if (size) {
    m_output += get_spacing_format_string(size);
  }
}

/**
 * Insert tag to alter the current font index
 */
void builder::font(int index) {
  if (index == 0) {
    return;
  }
  tag_open(syntaxtag::T, to_string(index));
}

/**
 * Insert tag to reset the font index
 */
void builder::font_close() {
  tag_close(syntaxtag::T);
}

/**
 * Insert tag to alter the current background color
 */
void builder::background(rgba color) {
  color = color.try_apply_alpha_to(m_bar.background);

  auto hex = color_util::simplify_hex(color);
  tag_open(syntaxtag::B, hex);
}

/**
 * Insert tag to reset the background color
 */
void builder::background_close() {
  tag_close(syntaxtag::B);
}

/**
 * Insert tag to alter the current foreground color
 */
void builder::foreground(rgba color) {
  color = color.try_apply_alpha_to(m_bar.foreground);

  auto hex = color_util::simplify_hex(color);
  tag_open(syntaxtag::F, hex);
}

/**
 * Insert tag to reset the foreground color
 */
void builder::foreground_close() {
  tag_close(syntaxtag::F);
}

/**
 * Close underline color tag
 */
void builder::overline_color_close() {
  tag_close(syntaxtag::o);
}

/**
 * Close underline color tag
 */
void builder::underline_color_close() {
  tag_close(syntaxtag::u);
}

/**
 * Insert tag to enable the overline attribute with the given color
 */
void builder::overline(const rgba& color) {
  if (color.has_color()) {
    auto hex = color_util::simplify_hex(color);
    tag_open(syntaxtag::o, hex);
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
 * Insert tag to enable the underline attribute with the given color
 */
void builder::underline(const rgba& color) {
  if (color.has_color()) {
    auto hex = color_util::simplify_hex(color);
    tag_open(syntaxtag::u, hex);
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
    case controltag::t:
      str = "t";
      break;
    default:
      throw runtime_error("Invalid controltag: " + to_string(to_integral(tag)));
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
    tag_open(syntaxtag::A, to_string(to_integral(index)) + ":" + action + ":");
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
    action_close();
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
    default:
      throw runtime_error("Invalid tag: " + to_string(to_integral(tag)));
  }
}

/**
 * Insert directive to use given attribute unless already set
 */
void builder::tag_open(attribute attr) {
  // Don't emit activation tag if the attribute is already activated
  if (m_attrs.count(attr) != 0) {
    return;
  }

  m_attrs.insert(attr);

  switch (attr) {
    case attribute::UNDERLINE:
      append("%{+u}");
      break;
    case attribute::OVERLINE:
      append("%{+o}");
      break;
    default:
      throw runtime_error("Invalid attribute: " + to_string(to_integral(attr)));
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
    default:
      throw runtime_error("Cannot close syntaxtag: " + to_string(to_integral(tag)));
  }
}

/**
 * Insert directive to remove given attribute if set
 */
void builder::tag_close(attribute attr) {
  // Don't close activation tag if it wasn't activated
  if (m_attrs.erase(attr) == 0) {
    return;
  }

  switch (attr) {
    case attribute::UNDERLINE:
      append("%{-u}");
      break;
    case attribute::OVERLINE:
      append("%{-o}");
      break;
    default:
      throw runtime_error("Invalid attribute: " + to_string(to_integral(attr)));
  }
}

string builder::get_spacing_format_string(spacing_val space) {
  float value = space.value;
  if (value == 0) {
    return "";
  }

  if (space.type == spacing_type::SPACE) {
    return string(value, ' ');
  } else {
    return "%{O" + units_utils::extent_to_string(units_utils::spacing_to_extent(space)) + "}";
  }
}

POLYBAR_NS_END

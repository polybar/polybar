#include "components/builder.hpp"

#include "drawtypes/label.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Flush contents of the builder and return built string
 *
 * This will also close any unclosed tags
 */
string builder::flush() {
  if (m_tags[syntaxtag::A])
    cmd_close();
  if (m_tags[syntaxtag::B])
    background_close();
  if (m_tags[syntaxtag::F])
    color_close();
  if (m_tags[syntaxtag::T])
    font_close();
  if (m_tags[syntaxtag::o])
    overline_color_close();
  if (m_tags[syntaxtag::u])
    underline_color_close();
  if ((m_attributes >> static_cast<uint8_t>(attribute::UNDERLINE)) & 1U)
    underline_close();
  if ((m_attributes >> static_cast<uint8_t>(attribute::OVERLINE)) & 1U)
    overline_close();

  string output = m_output.data();

  // reset values
  for (auto& counter : m_tags) counter.second = 0;
  for (auto& value : m_colors) value.second = "";
  m_output.clear();
  m_fontindex = 1;

  return string_util::replace_all(output, string{BUILDER_SPACE_TOKEN}, " ");
}

/**
 * Insert raw text string
 */
void builder::append(string text) {
  string str(text);
  size_t len{str.length()};
  if (len > 2 && str[0] == '"' && str[len - 1] == '"')
    m_output += str.substr(1, len - 2);
  else
    m_output += str;
}

/**
 * Insert text node
 *
 * This will also parse raw syntax tags
 */
void builder::node(string str, bool add_space) {
  string::size_type n, m;
  string s(str);

  while (true) {
    if (s.empty()) {
      break;

    } else if ((n = s.find("%{F-}")) == 0) {
      color_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{F#")) == 0 && (m = s.find("}")) != string::npos) {
      if (m - n - 4 == 2)
        color_alpha(s.substr(n + 3, m - 3));
      else
        color(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{B-}")) == 0) {
      background_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{B#")) == 0 && (m = s.find("}")) != string::npos) {
      background(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{T-}")) == 0) {
      font_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{T")) == 0 && (m = s.find("}")) != string::npos) {
      font(std::atoi(s.substr(n + 3, m - 3).c_str()));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{U-}")) == 0) {
      line_color_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{u-}")) == 0) {
      underline_color_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{o-}")) == 0) {
      overline_color_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{u#")) == 0 && (m = s.find("}")) != string::npos) {
      underline_color(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{o#")) == 0 && (m = s.find("}")) != string::npos) {
      overline_color(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{U#")) == 0 && (m = s.find("}")) != string::npos) {
      line_color(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{+u}")) == 0) {
      underline();
      s.erase(0, 5);

    } else if ((n = s.find("%{+o}")) == 0) {
      overline();
      s.erase(0, 5);

    } else if ((n = s.find("%{-u}")) == 0) {
      underline_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{-o}")) == 0) {
      overline_close();
      s.erase(0, 5);

    } else if ((n = s.find("%{A}")) == 0) {
      cmd_close();
      s.erase(0, 4);

    } else if ((n = s.find("%{")) == 0 && (m = s.find("}")) != string::npos) {
      append(s.substr(n, m + 1));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{")) > 0) {
      append(s.substr(0, n));
      s.erase(0, n);

    } else
      break;
  }

  if (!s.empty())
    append(s);
  if (add_space)
    space();
}

/**
 * Insert text node with specific font index
 *
 * @see builder::node
 */
void builder::node(string str, int font_index, bool add_space) {
  font(font_index);
  node(str, add_space);
  font_close();
}

/**
 * Insert tags for given label
 */
void builder::node(label_t label, bool add_space) {
  if (!label || !*label)
    return;

  string text{label->get()};

  if (label->m_maxlen > 0 && text.length() > label->m_maxlen) {
    text = text.substr(0, label->m_maxlen) + "...";
  }

  // if ((label->m_overline.empty() && m_tags[syntaxtag::o] > 0) || (m_tags[syntaxtag::o] > 0 && label->m_margin > 0))
  //   overline_close();
  // if ((label->m_underline.empty() && m_tags[syntaxtag::u] > 0) || (m_tags[syntaxtag::u] > 0 && label->m_margin > 0))
  //   underline_close();

  // TODO: Replace with margin-left
  if (label->m_margin > 0)
    space(label->m_margin);

  if (!label->m_overline.empty())
    overline(label->m_overline);
  if (!label->m_underline.empty())
    underline(label->m_underline);

  if (!label->m_background.empty())
    background(label->m_background);
  if (!label->m_foreground.empty())
    color(label->m_foreground);

  // TODO: Replace with padding-left
  if (label->m_padding > 0)
    space(label->m_padding);

  node(text, label->m_font, add_space);

  // TODO: Replace with padding-right
  if (label->m_padding > 0)
    space(label->m_padding);

  if (!label->m_background.empty())
    background_close();
  if (!label->m_foreground.empty())
    color_close();

  if (!label->m_underline.empty() || (label->m_margin > 0 && m_tags[syntaxtag::u] > 0))
    underline_close();
  if (!label->m_overline.empty() || (label->m_margin > 0 && m_tags[syntaxtag::o] > 0))
    overline_close();

  // TODO: Replace with margin-right
  if (label->m_margin > 0)
    space(label->m_margin);
}

/**
 * Insert tag that will offset the contents by given pixels
 */
void builder::offset(int pixels) {
  if (pixels == 0)
    return;
  tag_open(syntaxtag::O, to_string(pixels));
}

/**
 * Insert spaces
 */
void builder::space(int width) {
  if (width == DEFAULT_SPACING)
    width = m_bar.spacing;
  if (width <= 0)
    return;
  string str(width, ' ');
  append(str);
}

/**
 * Remove trailing space
 */
void builder::remove_trailing_space(int width) {
  if (width == DEFAULT_SPACING)
    width = m_bar.spacing;
  if (width <= 0)
    return;
  string::size_type spacing = width;
  string str(spacing, ' ');
  if (m_output.length() >= spacing && m_output.substr(m_output.length() - spacing) == str)
    m_output = m_output.substr(0, m_output.length() - spacing);
}

/**
 * Insert tag to alter the current font index
 */
void builder::font(int index) {
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
void builder::background(string color) {
  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    string bg{background_hex()};
    color = "#" + color.substr(color.length() - 2);
    color += bg.substr(bg.length() - (bg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#" + string(color.length() - 1, color[1])) {
    color = color.substr(0, 4);
  }

  color = color_util::simplify_hex(color);
  m_colors[syntaxtag::B] = color;
  tag_open(syntaxtag::B, color);
}

/**
 * Insert tag to reset the background color
 */
void builder::background_close() {
  m_colors[syntaxtag::B] = "";
  tag_close(syntaxtag::B);
}

/**
 * Insert tag to alter the current foreground color
 */
void builder::color(string color) {
  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    string fg{foreground_hex()};
    color = "#" + color.substr(color.length() - 2);
    color += fg.substr(fg.length() - (fg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#" + string(color.length() - 1, color[1])) {
    color = color.substr(0, 4);
  }

  color = color_util::simplify_hex(color);
  m_colors[syntaxtag::F] = color;
  tag_open(syntaxtag::F, color);
}

/**
 * Insert tag to alter the alpha value of the default foreground color
 */
void builder::color_alpha(string alpha) {
  string val{foreground_hex()};

  if (alpha.find("#") == std::string::npos) {
    alpha = "#" + alpha;
  }

  if (alpha.size() == 4) {
    color(alpha);
    return;
  }

  if (val.size() < 6 && val.size() > 2) {
    val.append(val.substr(val.size() - 3));
  }

  color((alpha.substr(0, 3) + val.substr(val.size() - 6)).substr(0, 9));
}

/**
 * Insert tag to reset the foreground color
 */
void builder::color_close() {
  m_colors[syntaxtag::F] = "";
  tag_close(syntaxtag::F);
}

/**
 * Insert tag to alter the current overline/underline color
 */
void builder::line_color(string color) {
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
void builder::overline_color(string color) {
  color = color_util::simplify_hex(color);
  m_colors[syntaxtag::o] = color;
  tag_open(syntaxtag::o, color);
  tag_open(attribute::OVERLINE);
}

/**
 * Close underline color tag
 */
void builder::overline_color_close() {
  m_colors[syntaxtag::o] = "";
  tag_close(syntaxtag::o);
}

/**
 * Insert tag to alter the current underline color
 */
void builder::underline_color(string color) {
  color = color_util::simplify_hex(color);
  m_colors[syntaxtag::u] = color;
  tag_open(syntaxtag::u, color);
  tag_open(attribute::UNDERLINE);
}

/**
 * Close underline color tag
 */
void builder::underline_color_close() {
  tag_close(syntaxtag::u);
  m_colors[syntaxtag::u] = "";
}

/**
 * Insert tag to enable the overline attribute
 */
void builder::overline(string color) {
  if (!color.empty())
    overline_color(color);
  else
    tag_open(attribute::OVERLINE);
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
void builder::underline(string color) {
  if (!color.empty())
    underline_color(color);
  else
    tag_open(attribute::UNDERLINE);
}

/**
 * Close underline attribute tag
 */
void builder::underline_close() {
  tag_close(attribute::UNDERLINE);
}

/**
 * Open command tag
 */
void builder::cmd(mousebtn index, string action, bool condition) {
  int button = static_cast<int>(index);

  if (!condition || action.empty())
    return;

  action = string_util::replace_all(action, ":", "\\:");
  action = string_util::replace_all(action, "$", "\\$");
  action = string_util::replace_all(action, "}", "\\}");
  action = string_util::replace_all(action, "{", "\\{");
  action = string_util::replace_all(action, "%", "\x0025");

  tag_open(syntaxtag::A, to_string(button) + ":" + action + ":");
}

/**
 * Close command tag
 */
void builder::cmd_close() {
  tag_close(syntaxtag::A);
}

/**
 * Get default background hex string
 */
string builder::background_hex() {
  if (m_background.empty())
    m_background = color_util::hex<uint16_t>(m_bar.background);
  return m_background;
}

/**
 * Get default foreground hex string
 */
string builder::foreground_hex() {
  if (m_foreground.empty())
    m_foreground = color_util::hex<uint16_t>(m_bar.foreground);
  return m_foreground;
}

/**
 * Insert directive to change value of given tag
 */
void builder::tag_open(syntaxtag tag, string value) {
  if (m_tags.find(tag) != m_tags.end())
    m_tags[tag]++;

  switch (tag) {
    case syntaxtag::NONE:
      break;
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
  }
}

/**
 * Insert directive to use given attribute unless already set
 */
void builder::tag_open(attribute attr) {
  if ((m_attributes >> static_cast<uint8_t>(attr)) & 1U)
    return;

  m_attributes |= 1U << static_cast<uint8_t>(attr);

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
  if (!m_tags[tag] || m_tags.find(tag) == m_tags.end())
    return;

  m_tags[tag]--;

  switch (tag) {
    case syntaxtag::NONE:
      break;
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
      break;
    case syntaxtag::O:
      break;
  }
}

/**
 * Insert directive to remove given attribute if set
 */
void builder::tag_close(attribute attr) {
  if (!((m_attributes >> static_cast<uint8_t>(attr)) & 1U))
    return;

  m_attributes &= ~(1U << static_cast<uint8_t>(attr));

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

POLYBAR_NS_END

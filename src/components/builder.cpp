#include "components/builder.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

void builder::set_lazy(bool mode) {
  m_lazy = mode;
}

string builder::flush() {
  if (m_lazy) {
    while (m_counters[syntaxtag::A] > 0) cmd_close(true);
    while (m_counters[syntaxtag::B] > 0) background_close(true);
    while (m_counters[syntaxtag::F] > 0) color_close(true);
    while (m_counters[syntaxtag::T] > 0) font_close(true);
    while (m_counters[syntaxtag::U] > 0) line_color_close(true);
    while (m_counters[syntaxtag::u] > 0) underline_close(true);
    while (m_counters[syntaxtag::o] > 0) overline_close(true);
  }

  string output = m_output.data();

  // reset values
  m_output.clear();
  for (auto& counter : m_counters) counter.second = 0;
  for (auto& value : m_colors) value.second = "";
  m_fontindex = 1;

  return string_util::replace_all(output, string{BUILDER_SPACE_TOKEN}, " ");
}

void builder::append(string text) {
  string str(text);
  auto len = str.length();
  if (len > 2 && str[0] == '"' && str[len - 1] == '"')
    m_output += str.substr(1, len - 2);
  else
    m_output += str;
}

void builder::node(string str, bool add_space) {
  string::size_type n, m;
  string s(str);

  while (true) {
    if (s.empty()) {
      break;

    } else if ((n = s.find("%{F-}")) == 0) {
      color_close(!m_lazy);
      s.erase(0, 5);

    } else if ((n = s.find("%{F#")) == 0 && (m = s.find("}")) != string::npos) {
      if (m - n - 4 == 2)
        color_alpha(s.substr(n + 3, m - 3));
      else
        color(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{B-}")) == 0) {
      background_close(!m_lazy);
      s.erase(0, 5);

    } else if ((n = s.find("%{B#")) == 0 && (m = s.find("}")) != string::npos) {
      background(s.substr(n + 3, m - 3));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{T-}")) == 0) {
      font_close(!m_lazy);
      s.erase(0, 5);

    } else if ((n = s.find("%{T")) == 0 && (m = s.find("}")) != string::npos) {
      font(std::atoi(s.substr(n + 3, m - 3).c_str()));
      s.erase(n, m + 1);

    } else if ((n = s.find("%{U-}")) == 0) {
      line_color_close(!m_lazy);
      s.erase(0, 5);

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
      underline_close(true);
      s.erase(0, 5);

    } else if ((n = s.find("%{-o}")) == 0) {
      overline_close(true);
      s.erase(0, 5);

    } else if ((n = s.find("%{A}")) == 0) {
      cmd_close(true);
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

void builder::node(string str, int font_index, bool add_space) {
  font(font_index);
  node(str, add_space);
  font_close();
}

// void builder::node(progressbar_t bar, float perc, bool add_space) {
//   if (!bar)
//     return;
//   node(bar->get_output(math_util::cap<float>(0, 100, perc)), add_space);
// }

void builder::node(label_t label, bool add_space) {
  if (!label || !*label)
    return;

  auto text = label->get();

  if (label->m_maxlen > 0 && text.length() > label->m_maxlen) {
    text = text.substr(0, label->m_maxlen) + "...";
  }

  if ((label->m_overline.empty() && m_counters[syntaxtag::o] > 0) ||
      (m_counters[syntaxtag::o] > 0 && label->m_margin > 0))
    overline_close(true);
  if ((label->m_underline.empty() && m_counters[syntaxtag::u] > 0) ||
      (m_counters[syntaxtag::u] > 0 && label->m_margin > 0))
    underline_close(true);

  if (label->m_margin > 0)
    space(label->m_margin);

  if (!label->m_overline.empty())
    overline(label->m_overline);
  if (!label->m_underline.empty())
    underline(label->m_underline);

  background(label->m_background);
  color(label->m_foreground);

  if (label->m_padding > 0)
    space(label->m_padding);

  node(text, label->m_font, add_space);

  if (label->m_padding > 0)
    space(label->m_padding);

  color_close(m_lazy && label->m_margin > 0);
  background_close(m_lazy && label->m_margin > 0);

  if (!label->m_underline.empty() || (label->m_margin > 0 && m_counters[syntaxtag::u] > 0))
    underline_close(m_lazy && label->m_margin > 0);
  if (!label->m_overline.empty() || (label->m_margin > 0 && m_counters[syntaxtag::o] > 0))
    overline_close(m_lazy && label->m_margin > 0);

  if (label->m_margin > 0)
    space(label->m_margin);
}

// void builder::node(ramp_t ramp, float perc, bool add_space) {
//   if (!ramp)
//     return;
//   node(ramp->get_by_percentage(math_util::cap<float>(0, 100, perc)), add_space);
// }

// void builder::node(animation_t animation, bool add_space) {
//   if (!animation)
//     return;
//   node(animation->get(), add_space);
// }

void builder::offset(int pixels) {
  if (pixels != 0)
    tag_open('O', std::to_string(pixels));
}

void builder::space(int width) {
  if (width == DEFAULT_SPACING)
    width = m_bar.spacing;
  if (width <= 0)
    return;
  string str(width, ' ');
  append(str);
}

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

void builder::invert() {
  tag_open('R', "");
}

void builder::font(int index) {
  if (index <= 0 && m_counters[syntaxtag::T] > 0)
    font_close(true);
  if (index <= 0 || index == m_fontindex)
    return;
  if (m_lazy && m_counters[syntaxtag::T] > 0)
    font_close(true);

  m_counters[syntaxtag::T]++;
  m_fontindex = index;
  tag_open('T', std::to_string(index));
}

void builder::font_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::T] <= 0)
    return;

  m_counters[syntaxtag::T]--;
  m_fontindex = 1;
  tag_close('T');
}

void builder::background(string color) {
  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    color = "#" + color.substr(color.length() - 2);
    auto bg = m_bar.background.source();
    color += bg.substr(bg.length() - (bg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#" + string(color.length() - 1, color[1])) {
    color = color.substr(0, 4);
  }

  if (color.empty() && m_counters[syntaxtag::B] > 0)
    background_close(true);
  if (color.empty() || color == m_colors[syntaxtag::B])
    return;
  if (m_lazy && m_counters[syntaxtag::B] > 0)
    background_close(true);

  m_counters[syntaxtag::B]++;
  m_colors[syntaxtag::B] = color;
  tag_open('B', color);
}

void builder::background_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::B] <= 0)
    return;

  m_counters[syntaxtag::B]--;
  m_colors[syntaxtag::B] = "";
  tag_close('B');
}

void builder::color(string color_) {
  auto color(color_);
  if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
    color = "#" + color.substr(color.length() - 2);
    auto fg = m_bar.foreground.source();
    color += fg.substr(fg.length() - (fg.length() < 6 ? 3 : 6));
  } else if (color.length() >= 7 && color == "#" + string(color.length() - 1, color[1])) {
    color = color.substr(0, 4);
  }

  if (color.empty() && m_counters[syntaxtag::F] > 0)
    color_close(true);
  if (color.empty() || color == m_colors[syntaxtag::F])
    return;
  if (m_lazy && m_counters[syntaxtag::F] > 0)
    color_close(true);

  m_counters[syntaxtag::F]++;
  m_colors[syntaxtag::F] = color;
  tag_open('F', color);
}

void builder::color_alpha(string alpha_) {
  auto alpha(alpha_);
  string val = m_bar.foreground.source();
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

void builder::color_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::F] <= 0)
    return;

  m_counters[syntaxtag::F]--;
  m_colors[syntaxtag::F] = "";
  tag_close('F');
}

void builder::line_color(string color) {
  if (color.empty() && m_counters[syntaxtag::U] > 0)
    line_color_close(true);
  if (color.empty() || color == m_colors[syntaxtag::U])
    return;
  if (m_lazy && m_counters[syntaxtag::U] > 0)
    line_color_close(true);

  m_counters[syntaxtag::U]++;
  m_colors[syntaxtag::U] = color;
  tag_open('U', color);
}

void builder::line_color_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::U] <= 0)
    return;

  m_counters[syntaxtag::U]--;
  m_colors[syntaxtag::U] = "";
  tag_close('U');
}

void builder::overline(string color) {
  if (!color.empty())
    line_color(color);
  if (m_counters[syntaxtag::o] > 0)
    return;

  m_counters[syntaxtag::o]++;
  append("%{+o}");
}

void builder::overline_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::o] <= 0)
    return;

  m_counters[syntaxtag::o]--;
  append("%{-o}");
}

void builder::underline(string color) {
  if (!color.empty())
    line_color(color);
  if (m_counters[syntaxtag::u] > 0)
    return;

  m_counters[syntaxtag::u]++;
  append("%{+u}");
}

void builder::underline_close(bool force) {
  if ((!force && m_lazy) || m_counters[syntaxtag::u] <= 0)
    return;

  m_counters[syntaxtag::u]--;
  append("%{-u}");
}

void builder::cmd(mousebtn index, string action, bool condition) {
  int button = static_cast<int>(index);

  if (!condition || action.empty())
    return;

  action = string_util::replace_all(action, ":", "\\:");
  action = string_util::replace_all(action, "$", "\\$");
  action = string_util::replace_all(action, "}", "\\}");
  action = string_util::replace_all(action, "{", "\\{");
  action = string_util::replace_all(action, "%", "\x0025");

  append("%{A" + std::to_string(button) + ":" + action + ":}");
  m_counters[syntaxtag::A]++;
}

void builder::cmd_close(bool force) {
  if (m_counters[syntaxtag::A] > 0 || force)
    append("%{A}");
  if (m_counters[syntaxtag::A] > 0)
    m_counters[syntaxtag::A]--;
}

void builder::tag_open(char tag, string value) {
  append("%{" + string({tag}) + value + "}");
}

void builder::tag_close(char tag) {
  append("%{" + string({tag}) + "-}");
}

LEMONBUDDY_NS_END

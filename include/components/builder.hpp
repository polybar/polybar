#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

#define DEFAULT_SPACING -1

#ifndef BUILDER_SPACE_TOKEN
#define BUILDER_SPACE_TOKEN "%__"
#endif

using namespace drawtypes;

class builder {
 public:
  explicit builder(const bar_settings bar, bool lazy = true) : m_bar(bar), m_lazy(lazy) {}

  void set_lazy(bool mode) {
    m_lazy = mode;
  }

  string flush() {
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

  void append(string text) {
    string str(text);
    auto len = str.length();
    if (len > 2 && str[0] == '"' && str[len - 1] == '"')
      m_output += str.substr(1, len - 2);
    else
      m_output += str;
  }

  void node(string str, bool add_space = false) {
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

  void node(string str, int font_index, bool add_space = false) {
    font(font_index);
    node(str, add_space);
    font_close();
  }

  // void node(progressbar_t bar, float perc, bool add_space = false) {
  //   if (!bar)
  //     return;
  //   node(bar->get_output(math_util::cap<float>(0, 100, perc)), add_space);
  // }

  void node(label_t label, bool add_space = false) {
    if (!label || !*label)
      return;

    auto text = label->m_text;

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

  // void node(ramp_t ramp, float perc, bool add_space = false) {
  //   if (!ramp)
  //     return;
  //   node(ramp->get_by_percentage(math_util::cap<float>(0, 100, perc)), add_space);
  // }

  // void node(animation_t animation, bool add_space = false) {
  //   if (!animation)
  //     return;
  //   node(animation->get(), add_space);
  // }

  void offset(int pixels = 0) {
    if (pixels != 0)
      tag_open('O', std::to_string(pixels));
  }

  void space(int width = DEFAULT_SPACING) {
    if (width == DEFAULT_SPACING)
      width = m_bar.spacing;
    if (width <= 0)
      return;
    string str(width, ' ');
    append(str);
  }

  void remove_trailing_space(int width = DEFAULT_SPACING) {
    if (width == DEFAULT_SPACING)
      width = m_bar.spacing;
    if (width <= 0)
      return;
    string::size_type spacing = width;
    string str(spacing, ' ');
    if (m_output.length() >= spacing && m_output.substr(m_output.length() - spacing) == str)
      m_output = m_output.substr(0, m_output.length() - spacing);
  }

  void invert() {
    tag_open('R', "");
  }

  void font(int index) {
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

  void font_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::T] <= 0)
      return;

    m_counters[syntaxtag::T]--;
    m_fontindex = 1;
    tag_close('T');
  }

  void background(string color) {
    if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
      color = "#" + color.substr(color.length() - 2);
      auto bg = m_bar.background.hex();
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

  void background_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::B] <= 0)
      return;

    m_counters[syntaxtag::B]--;
    m_colors[syntaxtag::B] = "";
    tag_close('B');
  }

  void color(string color_) {
    auto color(color_);
    if (color.length() == 2 || (color.find("#") == 0 && color.length() == 3)) {
      color = "#" + color.substr(color.length() - 2);
      auto bg = m_bar.foreground.hex();
      color += bg.substr(bg.length() - (bg.length() < 6 ? 3 : 6));
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

  void color_alpha(string alpha_) {
    auto alpha(alpha_);
    string val = m_bar.foreground.hex();
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

  void color_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::F] <= 0)
      return;

    m_counters[syntaxtag::F]--;
    m_colors[syntaxtag::F] = "";
    tag_close('F');
  }

  void line_color(string color) {
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

  void line_color_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::U] <= 0)
      return;

    m_counters[syntaxtag::U]--;
    m_colors[syntaxtag::U] = "";
    tag_close('U');
  }

  void overline(string color = "") {
    if (!color.empty())
      line_color(color);
    if (m_counters[syntaxtag::o] > 0)
      return;

    m_counters[syntaxtag::o]++;
    append("%{+o}");
  }

  void overline_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::o] <= 0)
      return;

    m_counters[syntaxtag::o]--;
    append("%{-o}");
  }

  void underline(string color = "") {
    if (!color.empty())
      line_color(color);
    if (m_counters[syntaxtag::u] > 0)
      return;

    m_counters[syntaxtag::u]++;
    append("%{+u}");
  }

  void underline_close(bool force = false) {
    if ((!force && m_lazy) || m_counters[syntaxtag::u] <= 0)
      return;

    m_counters[syntaxtag::u]--;
    append("%{-u}");
  }

  void cmd(mousebtn index, string action, bool condition = true) {
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

  void cmd_close(bool force = false) {
    if (m_counters[syntaxtag::A] > 0 || force)
      append("%{A}");
    if (m_counters[syntaxtag::A] > 0)
      m_counters[syntaxtag::A]--;
  }

 protected:
  void tag_open(char tag, string value) {
    append("%{" + string({tag}) + value + "}");
  }

  void tag_close(char tag) {
    append("%{" + string({tag}) + "-}");
  }

 private:
  const bar_settings m_bar;

  string m_output;
  bool m_lazy = true;

  map<syntaxtag, int> m_counters{
      // clang-format off
      {syntaxtag::A, 0},
      {syntaxtag::B, 0},
      {syntaxtag::F, 0},
      {syntaxtag::T, 0},
      {syntaxtag::U, 0},
      {syntaxtag::O, 0},
      {syntaxtag::R, 0},
      // clang-format on
  };

  map<syntaxtag, string> m_colors{
      // clang-format off
      {syntaxtag::B, ""},
      {syntaxtag::F, ""},
      {syntaxtag::U, ""},
      // clang-format on
  };

  int m_fontindex = 1;
};

LEMONBUDDY_NS_END;

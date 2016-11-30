#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "config.hpp"

POLYBAR_NS

#define DEFAULT_SPACING -1

#ifndef BUILDER_SPACE_TOKEN
#define BUILDER_SPACE_TOKEN "%__"
#endif

// fwd decl
namespace drawtypes {
  class label;
  using label_t = shared_ptr<label>;
  using icon = label;
  using icon_t = label_t;
}

using namespace drawtypes;

class builder {
 public:
  explicit builder(const bar_settings bar) : m_bar(bar) {}

  string flush();
  void append(string text);
  void node(string str, bool add_space = false);
  void node(string str, int font_index, bool add_space = false);
  void node(const label_t& label, bool add_space = false);
  void node_repeat(string str, size_t n, bool add_space = false);
  void node_repeat(const label_t& label, size_t n, bool add_space = false);
  void offset(int pixels = 0);
  void space(int width = DEFAULT_SPACING);
  void remove_trailing_space(int width = DEFAULT_SPACING);
  void font(int index);
  void font_close();
  void background(string color);
  void background_close();
  void color(string color);
  void color_alpha(string alpha);
  void color_close();
  void line_color(const string& color);
  void line_color_close();
  void overline_color(string color);
  void overline_color_close();
  void underline_color(string color);
  void underline_color_close();
  void overline(const string& color = "");
  void overline_close();
  void underline(const string& color = "");
  void underline_close();
  void cmd(mousebtn index, string action, bool condition = true);
  void cmd_close();

 protected:
  string background_hex();
  string foreground_hex();

  void tag_open(syntaxtag tag, const string& value);
  void tag_open(attribute attr);
  void tag_close(syntaxtag tag);
  void tag_close(attribute attr);

 private:
  const bar_settings m_bar;
  string m_output;

  map<syntaxtag, int> m_tags{
      // clang-format off
      {syntaxtag::A, 0},
      {syntaxtag::B, 0},
      {syntaxtag::F, 0},
      {syntaxtag::T, 0},
      {syntaxtag::u, 0},
      {syntaxtag::o, 0},
      // clang-format on
  };

  map<syntaxtag, string> m_colors{
      // clang-format off
      {syntaxtag::B, ""},
      {syntaxtag::F, ""},
      {syntaxtag::u, ""},
      {syntaxtag::o, ""},
      // clang-format on
  };

  uint8_t m_attributes{static_cast<uint8_t>(attribute::NONE)};
  uint8_t m_fontindex{1};

  string m_background;
  string m_foreground;
};

POLYBAR_NS_END

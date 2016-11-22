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
  explicit builder(const bar_settings bar, bool lazy = false) : m_bar(bar), m_lazy(lazy) {}

  void set_lazy(bool mode);

  string flush();

  void append(string text);

  void node(string str, bool add_space = false);
  void node(string str, int font_index, bool add_space = false);
  // void node(progressbar_t bar, float perc, bool add_space = false);
  void node(label_t label, bool add_space = false);
  // void node(ramp_t ramp, float perc, bool add_space = false);
  // void node(animation_t animation, bool add_space = false);

  void offset(int pixels = 0);
  void space(int width = DEFAULT_SPACING);
  void remove_trailing_space(int width = DEFAULT_SPACING);

  void invert();

  void font(int index);
  void font_close(bool force = false);

  void background(string color);
  void background_close(bool force = false);

  void color(string color_);
  void color_alpha(string alpha_);
  void color_close(bool force = false);

  void line_color(string color);
  void line_color_close(bool force = false);
  void overline_color(string color);
  void overline_color_close(bool force = false);
  void underline_color(string color);
  void underline_color_close(bool force = false);

  void overline(string color = "");
  void overline_close(bool force = false);

  void underline(string color = "");
  void underline_close(bool force = false);

  void cmd(mousebtn index, string action, bool condition = true);
  void cmd_close(bool force = false);

 protected:
  void tag_open(char tag, string value);
  void tag_close(char tag);

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
      {syntaxtag::Uo, 0},
      {syntaxtag::Uu, 0},
      {syntaxtag::O, 0},
      {syntaxtag::R, 0},
      // clang-format on
  };

  map<syntaxtag, string> m_colors{
      // clang-format off
      {syntaxtag::B, ""},
      {syntaxtag::F, ""},
      {syntaxtag::U, ""},
      {syntaxtag::Uu, ""},
      {syntaxtag::Uo, ""},
      // clang-format on
  };

  int m_fontindex = 1;
};

POLYBAR_NS_END

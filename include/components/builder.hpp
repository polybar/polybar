#pragma once

#include <map>

#include "common.hpp"
#include "components/types.hpp"

POLYBAR_NS

using std::map;

// fwd decl
using namespace drawtypes;

class builder {
 public:
  explicit builder(const bar_settings& bar);

  void reset();
  string flush();
  void append(string text);
  void node(string str);
  void node(string str, int font_index);
  void node(const label_t& label);
  void node_repeat(const string& str, size_t n);
  void node_repeat(const label_t& label, size_t n);
  void offset(int pixels = 0);
  void space(size_t width);
  void space();
  void remove_trailing_space(size_t len);
  void remove_trailing_space();
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
  void control(controltag tag);
  void cmd(mousebtn index, string action);
  void cmd(mousebtn index, string action, const label_t& label);
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

  map<syntaxtag, int> m_tags{};
  map<syntaxtag, string> m_colors{};
  map<attribute, bool> m_attrs{};

  int m_fontindex{0};

  string m_background{};
  string m_foreground{};
};

POLYBAR_NS_END

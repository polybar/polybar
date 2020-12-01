#pragma once

#include <map>

#include "common.hpp"
#include "components/types.hpp"
POLYBAR_NS

using std::map;

// fwd decl
using namespace drawtypes;
namespace modules {
  struct module_interface;
}

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
  void offset(int pixels);
  void space(size_t width);
  void space();
  void remove_trailing_space(size_t len);
  void remove_trailing_space();
  void font(int index);
  void font_close();
  void background(rgba color);
  void background_close();
  void color(rgba color);
  void color_close();
  void line_color(const rgba& color);
  void line_color_close();
  void overline_color(rgba color);
  void overline_color_close();
  void underline_color(rgba color);
  void underline_color_close();
  void overline(const rgba& color = rgba{});
  void overline_close();
  void underline(const rgba& color = rgba{});
  void underline_close();
  void control(controltag tag);
  void action(mousebtn index, string action);
  void action(mousebtn btn, const modules::module_interface& module, string action, string data);
  void action(mousebtn index, string action, const label_t& label);
  void action(mousebtn btn, const modules::module_interface& module, string action, string data, const label_t& label);
  void action_close();

 protected:

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
};

POLYBAR_NS_END

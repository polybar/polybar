#pragma once

#include <map>
#include <unordered_set>

#include "common.hpp"
#include "components/types.hpp"
#include "tags/types.hpp"
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
  void node(const string& str);
  void node(const string& str, int font_index);
  void node(const label_t& label);
  void node_repeat(const label_t& label, size_t n);
  void offset(extent_val pixels = ZERO_PX_EXTENT);
  void spacing(spacing_val size);
  void font(int index);
  void font_close();
  void background(rgba color);
  void background_close();
  void foreground(rgba color);
  void foreground_close();
  void overline(const rgba& color);
  void overline_close();
  void underline(const rgba& color);
  void underline_close();
  void control(tags::controltag tag);
  void action(mousebtn index, string action);
  void action(mousebtn btn, const modules::module_interface& module, string action, string data);
  void action(mousebtn index, string action, const label_t& label);
  void action(mousebtn btn, const modules::module_interface& module, string action, string data, const label_t& label);
  void action_close();

  static string get_spacing_format_string(spacing_val space);

 protected:
  void append(const string& text);

  void overline_color_close();
  void underline_color_close();

  void tag_open(tags::syntaxtag tag, const string& value);
  void tag_open(tags::attribute attr);
  void tag_close(tags::syntaxtag tag);
  void tag_close(tags::attribute attr);

 private:
  const bar_settings& m_bar;
  string m_output;

  map<tags::syntaxtag, int> m_tags{};
  std::unordered_set<tags::attribute> m_attrs{};
};

POLYBAR_NS_END

#pragma once

#include <map>

#include "common.hpp"

POLYBAR_NS

using std::map;

#define DEFAULT_SPACING -1

// fwd decl
namespace drawtypes {
  class label;
  using label_t = shared_ptr<label>;
  using icon = label;
  using icon_t = label_t;
}
using namespace drawtypes;

enum class alignment : uint8_t;
enum class attribute : uint8_t;
enum class edge : uint8_t;
enum class syntaxtag : uint8_t;
enum class mousebtn : uint8_t;
struct bar_settings;

class builder {
 public:
  explicit builder(const bar_settings& bar);

  string flush();
  void append(const string& text);
  void node(string str, bool add_space = false);
  void node(string str, int font_index, bool add_space = false);
  void node(const label_t& label, bool add_space = false);
  void node_repeat(const string& str, size_t n, bool add_space = false);
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
  void cmd_close(bool condition = true);

 protected:
  string background_hex();
  string foreground_hex();

  void tag_open(syntaxtag tag, const string& value);
  void tag_open(attribute attr);
  void tag_close(syntaxtag tag);
  void tag_close(attribute attr);

 private:
  const bar_settings& m_bar;
  string m_output;

  map<syntaxtag, int> m_tags;
  map<syntaxtag, string> m_colors;

  uint8_t m_attributes;
  uint8_t m_fontindex;

  string m_background;
  string m_foreground;
};

POLYBAR_NS_END

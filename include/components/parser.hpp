#pragma once

#include <stack>

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;
enum class attribute;
enum class mousebtn;
struct bar_settings;

DEFINE_ERROR(parser_error);
DEFINE_CHILD_ERROR(unrecognized_token, parser_error);
DEFINE_CHILD_ERROR(unrecognized_attribute, parser_error);
DEFINE_CHILD_ERROR(unclosed_actionblocks, parser_error);

class parser {
 public:
  using make_type = unique_ptr<parser>;
  static make_type make();

 public:
  explicit parser(signal_emitter& emitter);
  void parse(const bar_settings& bar, string data);

 protected:
  void codeblock(string&& data, const bar_settings& bar);
  size_t text(string&& data);

  unsigned int parse_color(std::stack<unsigned int>& color_stack, string& value, unsigned int fallback);
  unsigned int parse_color_string(const string& s, unsigned int fallback = 0);
  int parse_fontindex(const string& value);
  attribute parse_attr(const char attr);
  mousebtn parse_action_btn(const string& data);
  string parse_action_cmd(string&& data);

 private:
  signal_emitter& m_sig;
  vector<int> m_actions;
  unique_ptr<parser> m_parser;

  std::stack<unsigned int> m_fg;
  std::stack<unsigned int> m_bg;
  std::stack<unsigned int> m_ul;
  std::stack<unsigned int> m_ol;
  std::stack<int> m_fonts;
};

POLYBAR_NS_END

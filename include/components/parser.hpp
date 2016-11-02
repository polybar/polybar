#pragma once

#include "common.hpp"
#include "components/signals.hpp"
#include "components/types.hpp"

LEMONBUDDY_NS

DEFINE_ERROR(unrecognized_token);

class parser {
 public:
  explicit parser(const bar_settings& bar) : m_bar(bar) {}
  void operator()(string data);
  void codeblock(string data);
  size_t text(string data);

 protected:
  color parse_color(string s, color fallback = color{0});
  int parse_fontindex(string s);
  attribute parse_attr(const char s);
  mousebtn parse_action_btn(string data);
  string parse_action_cmd(string data);

 private:
  const bar_settings& m_bar;
  vector<int> m_actions;
};

LEMONBUDDY_NS_END

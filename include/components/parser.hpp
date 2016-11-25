#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class logger;
struct bar_settings;
enum class attribute : uint8_t;
enum class mousebtn : uint8_t;

DEFINE_ERROR(parser_error);
DEFINE_CHILD_ERROR(unrecognized_token, parser_error);
DEFINE_CHILD_ERROR(unrecognized_attribute, parser_error);
DEFINE_CHILD_ERROR(unclosed_actionblocks, parser_error);

class parser {
 public:
  explicit parser(const logger& logger, const bar_settings& bar);
  void operator()(string data);
  void codeblock(string data);
  size_t text(string data);

 protected:
  uint32_t parse_color(string s, uint32_t fallback = 0);
  int8_t parse_fontindex(string s);
  attribute parse_attr(const char attr);
  mousebtn parse_action_btn(string data);
  string parse_action_cmd(const string& data);

 private:
  const logger& m_log;
  const bar_settings& m_bar;
  vector<int> m_actions;
};

POLYBAR_NS_END

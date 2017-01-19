#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;
enum class attribute : uint8_t;
enum class mousebtn : uint8_t;
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

  uint32_t parse_color(const string& s, uint32_t fallback = 0);
  uint8_t parse_fontindex(const string& s);
  attribute parse_attr(const char attr);
  mousebtn parse_action_btn(const string& data);
  string parse_action_cmd(string&& data);

 private:
  signal_emitter& m_sig;
  vector<int> m_actions;
  unique_ptr<parser> m_parser;
};

POLYBAR_NS_END

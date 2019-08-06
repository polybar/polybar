#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;
enum class attribute;
enum class controltag;
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

  static unsigned int parse_color(const string& s, unsigned int fallback = 0);
  static int parse_fontindex(const string& s);
  static attribute parse_attr(const char attr);
  mousebtn parse_action_btn(const string& data);
  static string parse_action_cmd(string&& data);
  static controltag parse_control(const string& data);

 private:
  signal_emitter& m_sig;
  vector<int> m_actions;
  unique_ptr<parser> m_parser;
};

POLYBAR_NS_END

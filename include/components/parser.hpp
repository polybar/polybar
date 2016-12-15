#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

class signal_emitter;
struct bar_settings;
enum class attribute : uint8_t;
enum class mousebtn : uint8_t;

DEFINE_ERROR(parser_error);
DEFINE_CHILD_ERROR(unrecognized_token, parser_error);
DEFINE_CHILD_ERROR(unrecognized_attribute, parser_error);
DEFINE_CHILD_ERROR(unclosed_actionblocks, parser_error);

class parser {
 public:
  struct packet {
    uint16_t data[128]{0U};
    size_t length{0};
  };

  explicit parser(signal_emitter& emitter, const bar_settings& bar);
  void operator()(string data);

 protected:
  void codeblock(string&& data);
  size_t text(string&& data);

  uint32_t parse_color(const string& s, uint32_t fallback = 0);
  int8_t parse_fontindex(const string& s);
  attribute parse_attr(const char attr);
  mousebtn parse_action_btn(const string& data);
  string parse_action_cmd(string&& data);

 private:
  signal_emitter& m_sig;
  const bar_settings& m_bar;
  vector<int> m_actions;
};

POLYBAR_NS_END

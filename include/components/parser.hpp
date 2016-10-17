#pragma once

#include <fastdelegate/fastdelegate.hpp>

#include "common.hpp"
#include "components/types.hpp"
#include "components/signals.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

DEFINE_ERROR(unrecognized_token);

class parser {
 public:
  /**
   * Construct parser
   */
  explicit parser(const bar_settings& bar) : m_bar(bar) {}

  /**
   * Parse input data
   */
  void operator()(string data) {  // {{{
    size_t pos;

    while (data.length()) {
      if (data.compare(0, 2, "%{") == 0 && (pos = data.find("}")) != string::npos) {
        codeblock(data.substr(2, pos - 2));
        data.erase(0, pos + 1);
      } else {
        if ((pos = data.find("%{")) == string::npos)
          pos = data.length();
        data.erase(0, text(data.substr(0, pos)));
      }
    }
  }  // }}}

  /**
   * Parse contents in tag blocks, i.e: %{...}
   */
  void codeblock(string data) {  // {{{
    size_t pos;

    while (data.length()) {
      data = string_util::ltrim(data, ' ');

      if (data.empty())
        break;

      char tag = data[0];
      string value;

      // Remove the tag
      data.erase(0, 1);

      if ((pos = data.find_first_of(" }")) != string::npos)
        value = data.substr(0, pos);
      else
        value = data;

      switch (tag) {
        case 'B':
          // Ignore tag if it occurs again later in the same block
          if (data.find(" B") == string::npos && !g_signals::parser::color_change.empty())
            g_signals::parser::color_change.emit(gc::BG, parse_color(value, m_bar.background));
          break;

        case 'F':
          // Ignore tag if it occurs again later in the same block
          if (data.find(" F") == string::npos && !g_signals::parser::color_change.empty())
            g_signals::parser::color_change.emit(gc::FG, parse_color(value, m_bar.foreground));
          break;

        case 'U':
          // Ignore tag if it occurs again later in the same block
          if (data.find(" U") == string::npos && !g_signals::parser::color_change.empty()) {
            g_signals::parser::color_change.emit(gc::UL, parse_color(value, m_bar.linecolor));
            g_signals::parser::color_change.emit(gc::OL, parse_color(value, m_bar.linecolor));
          }
          break;

        case 'R':
          if (!g_signals::parser::color_change.empty()) {
            g_signals::parser::color_change.emit(gc::BG, m_bar.foreground);
            g_signals::parser::color_change.emit(gc::FG, m_bar.background);
          }
          break;

        case 'T':
          if (data.find(" T") == string::npos && !g_signals::parser::font_change.empty())
            g_signals::parser::font_change.emit(parse_fontindex(value));
          break;

        case 'O':
          if (!g_signals::parser::pixel_offset.empty())
            g_signals::parser::pixel_offset.emit(std::atoi(value.c_str()));
          break;

        case 'l':
          if (!g_signals::parser::alignment_change.empty())
            g_signals::parser::alignment_change.emit(alignment::LEFT);
          break;

        case 'c':
          if (!g_signals::parser::alignment_change.empty())
            g_signals::parser::alignment_change.emit(alignment::CENTER);
          break;

        case 'r':
          if (!g_signals::parser::alignment_change.empty())
            g_signals::parser::alignment_change.emit(alignment::RIGHT);
          break;

        case '+':
          if (!g_signals::parser::attribute_set.empty())
            g_signals::parser::attribute_set.emit(parse_attr(value[0]));
          break;

        case '-':
          if (!g_signals::parser::attribute_unset.empty())
            g_signals::parser::attribute_unset.emit(parse_attr(value[0]));
          break;

        case '!':
          if (!g_signals::parser::attribute_toggle.empty())
            g_signals::parser::attribute_toggle.emit(parse_attr(value[0]));
          break;

        case 'A':
          if (isdigit(data[0]) || data[0] == ':') {
            value = parse_action_cmd(data);
            mousebtn btn = parse_action_btn(data);
            m_actions.push_back(static_cast<int>(btn));

            if (!g_signals::parser::action_block_open.empty())
              g_signals::parser::action_block_open.emit(btn, value);

            // make sure we strip the correct length (btn+wrapping colons)
            if (data[0] != ':')
              value += "0";
            value += "::";
          } else if (!m_actions.empty()) {
            if (!g_signals::parser::action_block_close.empty())
              g_signals::parser::action_block_close.emit(parse_action_btn(data));
            m_actions.pop_back();
          }
          break;

        default:
          throw unrecognized_token(string{tag});
      }

      if (!data.empty())
        data.erase(0, !value.empty() ? value.length() : 1);
    }
  }  // }}}

  /**
   * Parse text strings
   */
  size_t text(string data) {  // {{{
    uint8_t* utf = (uint8_t*)data.c_str();

    // if (utf[0] < 0x80) {
    //   // grab all consecutive ascii chars
    //   size_t next_tag = data.find("%{");
    //   string sequence{(next_tag != string::npos) ? data.substr(0, next_tag) : data};
    //   size_t n = 0;
    //   while (sequence[n] != '\0' && static_cast<uint8_t>(sequence[n]) < 0x80 && ++n <= next_tag)
    //     ;
    //   g_signals::parser::ascii_text_write.emit(data.substr(0, n));
    //   return data.length();
    // }

    if (utf[0] < 0x80) {
      g_signals::parser::ascii_text_write.emit(utf[0]);
      return 1;
    } else if ((utf[0] & 0xe0) == 0xc0) {  // 2 byte utf-8 sequence
      g_signals::parser::unicode_text_write.emit((utf[0] & 0x1f) << 6 | (utf[1] & 0x3f));
      return 2;
    } else if ((utf[0] & 0xf0) == 0xe0) {  // 3 byte utf-8 sequence
      g_signals::parser::unicode_text_write.emit(
          (utf[0] & 0xf) << 12 | (utf[1] & 0x3f) << 6 | (utf[2] & 0x3f));
      return 3;
    } else if ((utf[0] & 0xf8) == 0xf0) {  // 4 byte utf-8 sequence
      g_signals::parser::unicode_text_write.emit(0xfffd);
      return 4;
    } else if ((utf[0] & 0xfc) == 0xf8) {  // 5 byte utf-8 sequence
      g_signals::parser::unicode_text_write.emit(0xfffd);
      return 5;
    } else if ((utf[0] & 0xfe) == 0xfc) {  // 6 byte utf-8 sequence
      g_signals::parser::unicode_text_write.emit(0xfffd);
      return 6;
    } else {  // invalid utf-8 sequence
      g_signals::parser::ascii_text_write.emit(utf[0]);
      return 1;
    }
  }  // }}}

 protected:
  color parse_color(string s, color fallback = color{0}) {  // {{{
    if (s.empty() || s == "-")
      return fallback;
    return color::parse(s, fallback);
  }  // }}}

  int parse_fontindex(string s) {  // {{{
    if (s.empty() || s == "-")
      return -1;
    char* p = (char*)s.c_str();
    return std::strtoul(p, &p, 10);
  }  // }}}

  attribute parse_attr(const char s) {  // {{{
    switch (s) {
      case 'o':
        return attribute::o;
        break;
      case 'u':
        return attribute::u;
        break;
    }
    return attribute::NONE;
  }  // }}}

  mousebtn parse_action_btn(string data) {  // {{{
    if (data[0] == ':')
      return mousebtn::LEFT;
    else if (isdigit(data[0]))
      return static_cast<mousebtn>(data[0] - '0');
    else if (!m_actions.empty())
      return static_cast<mousebtn>(m_actions.back());
    else
      return mousebtn::NONE;
  }  // }}}

  string parse_action_cmd(string data) {  // {{{
    auto start = string_util::find_nth(data, 0, ":", 1);
    auto end = string_util::find_nth(data, 0, ":", 2);
    return string_util::trim(data.substr(start, end), ':');
  }  // }}}

 private:
  const bar_settings& m_bar;
  vector<int> m_actions;
};

LEMONBUDDY_NS_END

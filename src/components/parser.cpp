#include <cassert>

#include "components/logger.hpp"
#include "components/parser.hpp"
#include "components/signals.hpp"
#include "components/types.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Construct parser instance
 */
parser::parser(const logger& logger, const bar_settings& bar) : m_log(logger), m_bar(bar) {
  assert(g_signals::parser::background_change != nullptr);
  assert(g_signals::parser::foreground_change != nullptr);
  assert(g_signals::parser::underline_change != nullptr);
  assert(g_signals::parser::overline_change != nullptr);
  assert(g_signals::parser::alignment_change != nullptr);
  assert(g_signals::parser::attribute_set != nullptr);
  assert(g_signals::parser::attribute_unset != nullptr);
  assert(g_signals::parser::attribute_toggle != nullptr);
  assert(g_signals::parser::font_change != nullptr);
  assert(g_signals::parser::pixel_offset != nullptr);
  assert(g_signals::parser::action_block_open != nullptr);
  assert(g_signals::parser::action_block_close != nullptr);
  assert(g_signals::parser::ascii_text_write != nullptr);
  assert(g_signals::parser::unicode_text_write != nullptr);
  assert(g_signals::parser::string_write != nullptr);
}

/**
 * Process input string
 */
void parser::operator()(string data) {
  size_t pos;

  m_log.trace_x("parser: %s", data);

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
}

/**
 * Process contents within tag blocks, i.e: %{...}
 */
void parser::codeblock(string data) {
  size_t pos;

  while (data.length()) {
    data = string_util::ltrim(data, ' ');

    if (data.empty())
      break;

    char tag{data[0]};
    string value;

    // Remove the tag
    data.erase(0, 1);

    if ((pos = data.find_first_of(" }")) != string::npos)
      value = data.substr(0, pos);
    else
      value = data;

    switch (tag) {
      case 'B':
        g_signals::parser::background_change(parse_color(value, m_bar.background));
        break;

      case 'F':
        g_signals::parser::foreground_change(parse_color(value, m_bar.foreground));
        break;

      case 'T':
        g_signals::parser::font_change(parse_fontindex(value));
        break;

      case 'U':
        g_signals::parser::underline_change(parse_color(value, m_bar.underline.color));
        g_signals::parser::overline_change(parse_color(value, m_bar.overline.color));
        break;

      case 'u':
        g_signals::parser::underline_change(parse_color(value, m_bar.underline.color));
        break;

      case 'o':
        g_signals::parser::overline_change(parse_color(value, m_bar.overline.color));
        break;

      case 'R':
        g_signals::parser::background_change(m_bar.foreground);
        g_signals::parser::foreground_change(m_bar.background);
        break;

      case 'O':
        g_signals::parser::pixel_offset(atoi(value.c_str()));
        break;

      case 'l':
        g_signals::parser::alignment_change(alignment::LEFT);
        break;

      case 'c':
        g_signals::parser::alignment_change(alignment::CENTER);
        break;

      case 'r':
        g_signals::parser::alignment_change(alignment::RIGHT);
        break;

      case '+':
        g_signals::parser::attribute_set(parse_attr(value[0]));
        break;

      case '-':
        g_signals::parser::attribute_unset(parse_attr(value[0]));
        break;

      case '!':
        g_signals::parser::attribute_toggle(parse_attr(value[0]));
        break;

      case 'A':
        if (isdigit(data[0]) || data[0] == ':') {
          value = parse_action_cmd(data);
          mousebtn btn = parse_action_btn(data);
          m_actions.push_back(static_cast<int>(btn));

          g_signals::parser::action_block_open(btn, value);

          // make sure we strip the correct length (btn+wrapping colons)
          if (value[0] != ':')
            value += "0";
          value += "::";
        } else if (!m_actions.empty()) {
          g_signals::parser::action_block_close(parse_action_btn(value));
          m_actions.pop_back();
        }
        break;

      default:
        throw unrecognized_token(string{tag});
    }

    if (!data.empty())
      data.erase(0, !value.empty() ? value.length() : 1);
  }
}

/**
 * Process text contents
 */
size_t parser::text(string data) {
  uint8_t* utf = (uint8_t*)data.c_str();

  if (utf[0] < 0x80) {
    // grab all consecutive ascii chars
    size_t next_tag = data.find("%{");
    if (next_tag != string::npos) {
      data.erase(next_tag);
    }
    size_t n = 0;
    while (utf[n] != '\0' && utf[++n] < 0x80)
      ;
    g_signals::parser::string_write(data.substr(0, n).c_str(), n);
    return n;
  } else if ((utf[0] & 0xe0) == 0xc0) {  // 2 byte utf-8 sequence
    g_signals::parser::unicode_text_write((utf[0] & 0x1f) << 6 | (utf[1] & 0x3f));
    return 2;
  } else if ((utf[0] & 0xf0) == 0xe0) {  // 3 byte utf-8 sequence
    g_signals::parser::unicode_text_write((utf[0] & 0xf) << 12 | (utf[1] & 0x3f) << 6 | (utf[2] & 0x3f));
    return 3;
  } else if ((utf[0] & 0xf8) == 0xf0) {  // 4 byte utf-8 sequence
    g_signals::parser::unicode_text_write(0xfffd);
    return 4;
  } else if ((utf[0] & 0xfc) == 0xf8) {  // 5 byte utf-8 sequence
    g_signals::parser::unicode_text_write(0xfffd);
    return 5;
  } else if ((utf[0] & 0xfe) == 0xfc) {  // 6 byte utf-8 sequence
    g_signals::parser::unicode_text_write(0xfffd);
    return 6;
  } else {  // invalid utf-8 sequence
    g_signals::parser::ascii_text_write(utf[0]);
    return 1;
  }
}

/**
 * Process color hex string and convert it to the correct value
 */
uint32_t parser::parse_color(string s, uint32_t fallback) {
  uint32_t color{0};
  if (s.empty() || s[0] == '-' || (color = color_util::parse(s, fallback)) == fallback)
    return fallback;
  return color_util::premultiply_alpha(color);
}

/**
 * Process font index and convert it to the correct value
 */
int8_t parser::parse_fontindex(string s) {
  if (s.empty() || s[0] == '-') {
    return -1;
  }

  try {
    return std::stoul(s.c_str(), nullptr, 10);
  } catch (const std::invalid_argument& err) {
    return -1;
  }
}

/**
 * Process attribute token and convert it to the correct value
 */
attribute parser::parse_attr(const char attr) {
  switch (attr) {
    case 'o':
      return attribute::OVERLINE;
    case 'u':
      return attribute::UNDERLINE;
    default:
      throw unrecognized_attribute(string{attr});
  }
}

/**
 * Process action button token and convert it to the correct value
 */
mousebtn parser::parse_action_btn(string data) {
  if (data[0] == ':')
    return mousebtn::LEFT;
  else if (isdigit(data[0]))
    return static_cast<mousebtn>(data[0] - '0');
  else if (!m_actions.empty())
    return static_cast<mousebtn>(m_actions.back());
  else
    return mousebtn::NONE;
}

/**
 * Process action command string
 */
string parser::parse_action_cmd(string data) {
  size_t start, end;
  if ((start = data.find(':')) == string::npos)
    return "";
  if ((end = data.find(':', start + 1)) == string::npos)
    return "";
  return string_util::trim(data.substr(start, end), ':');
}

POLYBAR_NS_END

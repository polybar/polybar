#include "tags/parser.hpp"

#include <cassert>
#include <cctype>

#include "utils/units.hpp"

POLYBAR_NS

namespace tags {

  bool parser::has_next_element() {
    return buf_pos < buf.size() || has_next();
  }

  element parser::next_element() {
    if (!has_next_element()) {
      throw std::runtime_error("tag parser: No next element");
    }

    if (buf_pos >= buf.size()) {
      parse_step();
    }

    if (buf_pos >= buf.size()) {
      throw std::runtime_error("tag parser: No next element. THIS IS A BUG. (Context: '" + input + "')");
    }

    element e = buf[buf_pos];
    buf_pos++;

    if (buf_pos == buf.size()) {
      buf.clear();
      buf_pos = 0;
    }

    return e;
  }

  format_string parser::parse() {
    format_string parsed;

    while (has_next_element()) {
      parsed.push_back(next_element());
    }

    return parsed;
  }

  /**
   * Performs a single parse step.
   *
   * This means it will parse text until the next tag is reached or it will
   * parse an entire %{...} tag.
   */
  void parser::parse_step() {
    char c;

    /*
     * If we have already parsed text, we can stop if we reach a tag.
     */
    bool text_parsed = false;

    size_t start_pos = pos;

    try {
      while ((c = next())) {
        // TODO here we could think about how to escape an action tag
        if (c == '%' && has_next() && peek() == '{') {
          /*
           * If we have already parsed text, encountering a tag opening means
           * we can stop parsing now because we parsed at least one entire
           * element (the text up to the beginning of the tag).
           */
          if (text_parsed) {
            // Put back the '%'
            revert();
            break;
          }

          consume('{');
          consume_space();
          parse_tag();
          break;
        } else {
          push_char(c);
          text_parsed = true;
        }
      }
    } catch (error& e) {
      e.set_context(input.substr(start_pos, pos - start_pos));
      throw;
    }
  }

  void parser::set(const string&& input) {
    this->input = std::move(input);
    pos = 0;
    buf.clear();
    buf_pos = 0;
  }

  bool parser::has_next() const {
    return pos < input.size();
  }

  char parser::next() {
    char c = peek();
    pos++;
    return c;
  }

  char parser::peek() const {
    if (!has_next()) {
      return EOL;
    }

    return input[pos];
  }

  /**
   * Puts back a single character in the input string.
   */
  void parser::revert() {
    assert(pos > 0);
    pos--;
  }

  void parser::consume(char c) {
    char n = next();
    if (n != c) {
      throw tags::token_error(n, c);
    }
  }

  void parser::consume_space() {
    while (peek() == ' ') {
      next();
    }
  }

  /**
   * Parses an entire %{....} tag.
   *
   * '%' and '{' were already consumed and we are currently on the first character
   * inside the tag.
   * At the end of this method, we should be on the closing '}' character (not
   * yet consumed).
   */

  void parser::parse_tag() {
    if (!has_next()) {
      throw token_error(EOL, "Formatting tag content");
    }

    while (has_next()) {
      parse_single_tag_content();

      int p = peek();

      if (p != ' ' && p != '}') {
        throw tag_end_error(p);
      } else {
        /**
         * Consume whitespace between elements inside the tag
         */
        consume_space();

        if (peek() == '}') {
          consume('}');
          break;
        }
      }
    }
  }

  /**
   * Parses a single element inside a formatting tag.
   *
   * For example it would parse the foreground part of the following tag:
   *
   * %{F#ff0000 B#ff0000}
   *   ^       ^
   *   |       - Pointer at the end
   *   |
   *   - Pointer at the start
   */
  void parser::parse_single_tag_content() {
    char c = next();

    /**
     * %{U...} is a special case because it produces over and underline tags.
     */
    if (c == 'U') {
      element e{};
      e.is_tag = true;
      e.tag_data.type = tag_type::FORMAT;
      e.tag_data.subtype.format = syntaxtag::u;
      e.tag_data.color = parse_color();
      buf.emplace_back(e);

      e.tag_data.subtype.format = syntaxtag::o;
      buf.emplace_back(e);
      return;
    }

    tag_type type;
    tag_subtype sub;

    switch (c) {
      // clang-format off
      case 'B': sub.format = syntaxtag::B; break;
      case 'F': sub.format = syntaxtag::F; break;
      case 'u': sub.format = syntaxtag::u; break;
      case 'o': sub.format = syntaxtag::o; break;
      case 'T': sub.format = syntaxtag::T; break;
      case 'R': sub.format = syntaxtag::R; break;
      case 'O': sub.format = syntaxtag::O; break;
      case 'P': sub.format = syntaxtag::P; break;
      case 'A': sub.format = syntaxtag::A; break;
      case 'l': sub.format = syntaxtag::l; break;
      case 'c': sub.format = syntaxtag::c; break;
      case 'r': sub.format = syntaxtag::r; break;

      case '+': sub.activation = attr_activation::ON; break;
      case '-': sub.activation = attr_activation::OFF; break;
      case '!': sub.activation = attr_activation::TOGGLE; break;

        // clang-format on

      default:
        throw unrecognized_tag(c);
    }

    switch (c) {
      case 'B':
      case 'F':
      case 'u':
      case 'o':
      case 'T':
      case 'R':
      case 'O':
      case 'P':
      case 'A':
      case 'l':
      case 'c':
      case 'r':
        type = tag_type::FORMAT;
        break;

      case '+':
      case '-':
      case '!':
        type = tag_type::ATTR;
        break;

      default:
        throw unrecognized_tag(c);
    }

    tag tag_data{};
    tag_data.type = type;
    tag_data.subtype = sub;

    element e{};
    e.is_tag = true;

    switch (c) {
      case 'B':
      case 'F':
      case 'u':
      case 'o':
        tag_data.color = parse_color();
        break;
      case 'T':
        tag_data.font = parse_fontindex();
        break;
      case 'O':
        tag_data.offset = parse_offset();
        break;
      case 'P':
        tag_data.ctrl = parse_control();
        break;
      case 'A':
        std::tie(tag_data.action, e.data) = parse_action();
        break;

      case '+':
      case '-':
      case '!':
        tag_data.attr = parse_attribute();
        break;
    }

    e.tag_data = tag_data;
    buf.emplace_back(e);
  }

  color_value parser::parse_color() {
    string s = get_tag_value();

    color_value ret;

    if (s.empty() || s == "-") {
      ret.type = color_type::RESET;
    } else {
      rgba c{s};

      if (!c.has_color()) {
        throw color_error(s);
      }

      ret.type = color_type::COLOR;
      ret.val = c;
    }

    return ret;
  }

  int parser::parse_fontindex() {
    string s = get_tag_value();

    if (s.empty() || s == "-") {
      return 0;
    }

    try {
      size_t ptr;
      int ret = std::stoi(s, &ptr, 10);

      if (ret < 0) {
        return 0;
      }

      if (ptr != s.size()) {
        throw font_error(s, "Font index contains non-number characters");
      }

      return ret;
    } catch (const std::exception& err) {
      throw font_error(s, err.what());
    }
  }

  extent_val parser::parse_offset() {
    string s = get_tag_value();

    if (s.empty()) {
      return ZERO_PX_EXTENT;
    }

    try {
      return units_utils::parse_extent(string{s});
    } catch (const std::exception& err) {
      throw offset_error(s, err.what());
    }
  }

  controltag parser::parse_control() {
    string s = get_tag_value();

    if (s.empty()) {
      throw control_error(s, "Control tag is empty");
    }

    switch (s[0]) {
      case 'R':
        if (s.size() != 1) {
          throw control_error(s, "Control tag R has extra data");
        }

        return controltag::R;
      case 't':
        return controltag::t;
      default:
        throw control_error(s);
    }
  }

  std::pair<action_value, string> parser::parse_action() {
    mousebtn btn = parse_action_btn();

    action_value ret;

    string cmd;

    if (has_next() && peek() == ':') {
      ret.btn = btn == mousebtn::NONE ? mousebtn::LEFT : btn;
      ret.closing = false;
      cmd = parse_action_cmd();
    } else {
      ret.btn = btn;
      ret.closing = true;
    }

    return {ret, cmd};
  }

  /**
   * Parses the button index after starting an action tag.
   *
   * May return mousebtn::NONE if no button was given.
   */
  mousebtn parser::parse_action_btn() {
    if (has_next()) {
      if (isdigit(peek())) {
        char c = next();
        int num = c - '0';

        if (num < static_cast<int>(mousebtn::NONE) || num >= static_cast<int>(mousebtn::BTN_COUNT)) {
          throw btn_error(string{c});
        }

        return static_cast<mousebtn>(num);
      }
    }

    return mousebtn::NONE;
  }

  /**
   * Starts at ':' and parses a complete action string.
   *
   * Returns the parsed action string with without escaping backslashes.
   *
   * Afterwards the parsers will be at the character immediately after the
   * closing colon.
   */
  string parser::parse_action_cmd() {
    consume(':');

    string s;

    char prev = EOL;

    while (has_next()) {
      char c = next();

      if (c == ':') {
        if (prev == '\\') {
          s.pop_back();
          s.push_back(c);
        } else {
          break;
        }
      } else {
        s.push_back(c);
      }

      prev = c;
    }

    return s;
  }

  attribute parser::parse_attribute() {
    char c;
    switch (c = next()) {
      case 'u':
        return attribute::UNDERLINE;
      case 'o':
        return attribute::OVERLINE;
      default:
        throw unrecognized_attr(c);
    }
  }

  void parser::push_char(char c) {
    if (!buf.empty() && buf_pos < buf.size() && !buf.back().is_tag) {
      buf.back().data += c;
    } else {
      buf.emplace_back(string{c});
    }
  }

  void parser::push_text(string&& text) {
    if (text.empty()) {
      return;
    }

    if (!buf.empty() && buf_pos < buf.size() && !buf.back().is_tag) {
      buf.back().data += text;
    } else {
      buf.emplace_back(std::move(text));
    }
  }

  /**
   * Will read up until the end of the tag value.
   *
   * Afterwards the parser will be at the character directly after the tag
   * value.
   *
   * This function just reads until it encounters a space or a closing curly
   * bracket, so it is not useful for tag values that can contain these
   * characters (e.g. action tags).
   */
  string parser::get_tag_value() {
    string s;

    while (has_next() && peek() != ' ' && peek() != '}') {
      s.push_back(next());
    }

    return s;
  }
} // namespace tags

POLYBAR_NS_END

#include "tags/parser.hpp"

#include <cassert>

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
      throw std::runtime_error("tag parser: No next element. THIS IS A BUG. Context: '" + input + "'");
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
          if (text_parsed) {
            // Put back the '%'
            revert();
            break;
          }

          consume('{');
          parse_tag();
          consume('}');
          break;
        } else {
          push_char(c);
          text_parsed = true;
        }
      }
    } catch (error& e) {
      e.context = input.substr(start_pos, pos);
      throw e;
    }
  }

  void parser::set(const string&& input) {
    this->input = std::move(input);
    pos = 0;
    buf.clear();
    buf_pos = 0;
  }

  bool parser::has_next() {
    return pos < input.size();
  }

  char parser::next() {
    char c = peek();
    pos++;
    return c;
  }

  char parser::peek() {
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

      if (peek() != ' ') {
        break;
      }

      /**
       * Consume whitespace between elements inside the tag
       */
      while (peek() == ' ') {
        consume(' ');
      }

      /**
       * Break if there was whitespace at the end of the tag.
       */
      if (peek() == '}') {
        break;
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

    // TODO add special check for 'U' tag

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

      case '+': sub.activation = attr_activation::ON; break;
      case '-': sub.activation = attr_activation::OFF; break;
      case '!': sub.activation = attr_activation::TOGGLE; break;

      case 'l': sub.align = alignment::LEFT; break;
      case 'c': sub.align = alignment::CENTER; break;
      case 'r': sub.align = alignment::RIGHT; break;
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
        type = tag_type::FORMAT;
        break;

      case '+':
      case '-':
      case '!':
        type = tag_type::ATTR;
        break;

      case 'l':
      case 'c':
      case 'r':
        type = tag_type::ALIGN;
        break;
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
  }

  color_value parser::parse_color() {
    // TODO
    return color_value{};
  }

  unsigned parser::parse_fontindex() {
    // TODO
    return 0;
  }

  int parser::parse_offset() {
    // TODO
    return 0;
  }

  controltag parser::parse_control() {
    // TODO
    return controltag::NONE;
  }

  std::pair<action_value, string> parser::parse_action() {
    // TODO
    return {action_value{}, ""};
  }

  attribute parser::parse_attribute() {
    char c;
    switch (c = next()) {
      case 'u':
        return attribute::UNDERLINE;
      case 'o':
        return attribute::OVERLINE;
      default:
        throw token_error(c, "");
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

}  // namespace tags

POLYBAR_NS_END

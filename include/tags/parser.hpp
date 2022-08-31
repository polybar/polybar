#pragma once

#include "common.hpp"
#include "errors.hpp"
#include "tags/types.hpp"

POLYBAR_NS

namespace tags {

  static constexpr char EOL = '\0';

  class error : public application_error {
   public:
    using application_error::application_error;

    explicit error(const string& msg) : application_error(msg), msg(msg) {}

    /**
     * Context string that contains the text region where the parser error
     * happened.
     */
    void set_context(const string& ctxt) {
      msg.append(" (Context: '" + ctxt + "')");
    }

    virtual const char* what() const noexcept override {
      return msg.c_str();
    }

   private:
    string msg;
  };

#define DEFINE_INVALID_ERROR(class_name, name)                                           \
  class class_name : public error {                                                      \
   public:                                                                               \
    explicit class_name(const string& val) : error("Invalid " name ": '" + val + "'") {} \
    explicit class_name(const string& val, const string& what)                           \
        : error("Invalid " name ": '" + val + "' (reason: '" + what + "')") {}           \
  }

  DEFINE_INVALID_ERROR(color_error, "color");
  DEFINE_INVALID_ERROR(font_error, "font index");
  DEFINE_INVALID_ERROR(control_error, "control tag");
  DEFINE_INVALID_ERROR(offset_error, "offset");
  DEFINE_INVALID_ERROR(btn_error, "button id");
#undef DEFINE_INVALID_ERROR

  class token_error : public error {
   public:
    explicit token_error(char token, char expected) : token_error(string{token}, string{expected}) {}
    explicit token_error(char token, const string& expected) : token_error(string{token}, expected) {}
    explicit token_error(const string& token, const string& expected)
        : error("Expected '" + expected + "' but found '" +
                (token.size() == 1 && token.at(0) == EOL ? "<End Of Line>" : token) + "'") {}
  };

  class unrecognized_tag : public error {
   public:
    explicit unrecognized_tag(char tag) : error("Unrecognized formatting tag '%{" + string{tag} + "}'") {}
  };

  class unrecognized_attr : public error {
   public:
    explicit unrecognized_attr(char attr) : error("Unrecognized attribute '" + string{attr} + "'") {}
  };

  /**
   * Thrown when we expect the end of a tag (either } or a space in a compound
   * tag.
   */
  class tag_end_error : public error {
   public:
    explicit tag_end_error(char token)
        : error("Expected the end of a tag ('}' or ' ') but found '" +
                (token == EOL ? "<End Of Line>" : string{token}) + "'") {}
  };

  /**
   * Recursive-descent parser for polybar's formatting tags.
   *
   * An input string is parsed into a list of elements, each element is either
   * a piece of text or a single formatting tag.
   *
   * The elements can either be retrieved one-by-one with next_element() or all
   * at once with parse().
   */
  class parser {
   public:
    /**
     * Resets the parser state and sets the new string to parse
     */
    void set(const string&& input);

    /**
     * Whether a call to next_element() suceeds.
     */
    bool has_next_element();

    /**
     * Parses at least one element (if available) and returns the first parsed
     * element.
     */
    element next_element();

    /**
     * Parses the remaining string and returns all parsed elements.
     */
    format_string parse();

   protected:
    void parse_step();

    bool has_next() const;
    char next();
    char peek() const;
    void revert();

    void consume(char c);
    void consume_space();

    void parse_tag();

    void parse_single_tag_content();

    color_value parse_color();
    int parse_fontindex();
    extent_val parse_offset();
    controltag parse_control();
    std::pair<action_value, string> parse_action();
    mousebtn parse_action_btn();
    string parse_action_cmd();
    attribute parse_attribute();

    void push_char(char c);
    void push_text(string&& text);

    string get_tag_value();

   private:
    string input;
    size_t pos = 0;

    /**
     * Buffers elements that have been parsed but not yet returned to the user.
     */
    format_string buf{};
    /**
     * Index into buf so that we don't have to call vector.erase everytime.
     *
     * Only buf[buf_pos, buf.end()) contain valid elements
     */
    size_t buf_pos;
  };

}  // namespace tags

POLYBAR_NS_END

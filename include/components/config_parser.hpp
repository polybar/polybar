#pragma once

#include <set>

#include "common.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "errors.hpp"

POLYBAR_NS

DEFINE_ERROR(parser_error);

/**
 * @brief Exception object for syntax errors
 *
 * Contains filepath and line number where syntax error was found
 */
class syntax_error : public parser_error {
 public:
  /**
   * Default values are used when the thrower doesn't know the position.
   * parse_line has to catch, set the proper values and rethrow
   */
  explicit syntax_error(string msg, const string& file = "", int line_no = -1)
      : parser_error(file + ":" + to_string(line_no) + ": " + msg), msg(move(msg)) {}

  const string& get_msg() {
    return msg;
  };

 private:
  string msg;
};

class invalid_name_error : public syntax_error {
 public:
  /**
   * type is either Header or Key
   */
  invalid_name_error(const string& type, const string& name)
      : syntax_error(type + " name '" + name + "' is empty or contains forbidden characters.") {}

  invalid_name_error(const string& type, const string& name, const string& file, int line_no)
      : syntax_error(type + " name '" + name + "' is empty or contains forbidden characters.", file, line_no) {}
};

/**
 * @brief All different types a line in a config can be
 */
enum class line_type { KEY, HEADER, COMMENT, EMPTY, UNKNOWN };

/**
 * @brief Storage for a single config line
 *
 * More sanitized than the actual string of the comment line, with information
 * about line type and structure
 */
struct line_t {
  /**
   * Whether or not this struct represents a "useful" line, a line that has
   * any semantic significance (key-value or header line)
   * If false all other fields are not set.
   * Set this to false, if you want to return a line that has no effect
   * (for example when you parse a comment line)
   */
  bool useful;

  /**
   * Index of the config_parser::files vector where this line is from
   */
  int file_index;
  int line_no;

  /**
   * We access header, if is_header == true otherwise we access key, value
   */
  bool is_header;

  /**
   * Only set for header lines
   */
  string header;

  /**
   * Only set for key-value lines
   */
  string key, value;
};

class config_parser {
 public:
  config_parser(const logger& logger, string&& file);
  /**
   * This prevents passing a temporary logger to the constructor because that would be UB, as the temporary would be
   * destroyed once the constructor returns.
   */
  config_parser(logger&& logger, string&& file) = delete;

  /**
   * @brief Performs the parsing of the main config file m_file
   *
   * @returns config class instance populated with the parsed config
   *
   * @throws syntax_error If there was any kind of syntax error
   * @throws parser_error If aynthing else went wrong
   */
  config parse(string barname);

 protected:
  /**
   * @brief Converts the `lines` vector to a proper sectionmap
   */
  sectionmap_t create_sectionmap();

  /**
   * @brief Parses the given file, extracts key-value pairs and section
   *        headers and adds them onto the `lines` vector
   *
   * This method directly resolves `include-file` directives and checks for
   * cyclic dependencies
   *
   * `file` is expected to be an already resolved absolute path
   */
  void parse_file(const string& file, file_list path);

  /**
   * @brief Parses the given line string to the given line_t struct
   *
   * We use the INI file syntax (https://en.wikipedia.org/wiki/INI_file)
   * Whitespaces (tested with isspace()) at the beginning and end of a line are ignored
   * Keys and section names can contain any character except for the following:
   * - spaces
   * - equal sign (=)
   * - semicolon (;)
   * - pound sign (#)
   * - Any kind of parentheses ([](){})
   * - colon (:)
   * - period (.)
   * - dollar sign ($)
   * - backslash (\)
   * - percent sign (%)
   * - single and double quotes ('")
   * So basically any character that has any kind of special meaning is prohibited.
   *
   * Comment lines have to start with a semicolon (;) or a pound sign (#),
   * you cannot put a comment after another type of line.
   *
   * key and section names are case-sensitive.
   *
   * Keys are specified as `key = value`, spaces around the equal sign, as
   * well as double quotes around the value are ignored
   *
   * sections are defined as [section], everything inside the square brackets is part of the name
   *
   * @throws syntax_error if the line isn't well formed. The syntax error
   *         does not contain the filename or line numbers because parse_line
   *         doesn't know about those. Whoever calls parse_line needs to
   *         catch those exceptions and set the file path and line number
   */
  void parse_line(line_t& line, const string& line_str);

  /**
   * @brief Determines the type of a line read from a config file
   *
   * Expects that line is trimmed
   * This mainly looks at the first character and doesn't check if the line is
   * actually syntactically correct.
   * HEADER ('['), COMMENT (';' or '#') and EMPTY (None) are uniquely
   * identified by their first character (or lack thereof). Any line that
   * is none of the above and contains an equal sign, is treated as KEY.
   * All others are UNKNOWN
   */
  static line_type get_line_type(const string& line);

  /**
   * @brief Parse a line containing a section header and returns the header name
   *
   * Only assumes that the line starts with '[' and is trimmed
   *
   * @throws syntax_error if the line doesn't end with ']' or the header name
   *         contains forbidden characters
   */
  string parse_header(const line_t& line, const string& line_str);

  /**
   * @brief Parses a line containing a key-value pair and returns the key name
   *        and the value string inside an std::pair
   *
   * Only assumes that the line contains '=' at least once and is trimmed
   *
   * @throws syntax_error if the key contains forbidden characters
   */
  std::pair<string, string> parse_key(const line_t& line, const string& line_str);

  /**
   * @brief Parses the given value, checks if the given value contains
   *        one or more unescaped backslashes and logs an error if yes
   */
  string parse_escaped_value(const line_t& line, string&& value, const string& key);

  /**
   * @brief Name of all the files the config includes values from
   *
   * The line_t struct uses indices to this vector to map lines to their
   * original files. This allows us to point the user to the exact location
   * of errors
   */
  file_list m_files;

 private:
  /**
   * @brief Checks if the given name doesn't contain any spaces or characters
   *        in config_parser::m_forbidden_chars
   */
  bool is_valid_name(const string& name);

  vector<string> get_bars(const sectionmap_t& sections) const;

  /**
   * @brief Whether or not an xresource manager should be used
   *
   * Is set to true if any ${xrdb...} references are found
   */
  bool use_xrm{false};

  const logger& m_log;

  /**
   * @brief Absolute path to the main config file
   */
  string m_config_file;

  /**
   * @brief List of all the lines in the config (with included files)
   *
   * The order here matters, as we have not yet associated key-value pairs
   * with sections
   */
  vector<line_t> m_lines;

  /**
   * @brief None of these characters can be used in the key and section names
   */
  const string m_forbidden_chars{"\"'=;#[](){}:.$\\%"};

  /**
   * @brief List of names that cannot be used as section names
   *
   * These strings have a special meaning inside references and so the
   * section [self] could never be referenced.
   *
   * Note: BAR is deprecated
   */
  const std::set<string> m_reserved_section_names = {"self", "BAR", "root"};
};

POLYBAR_NS_END

#pragma once

#include <algorithm>
#include <unordered_map>

#include "common.hpp"
#include "errors.hpp"
#include "components/logger.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * \brief Generic exception for the config_parser
 */
DEFINE_ERROR(parser_error);

/**
 * \brief Exception object for syntax errors
 *
 * Contains filepath and line number where syntax error was found
 */
class syntax_error : public parser_error {

  public:
    /**
     * Default values are used when the thrower doesn't know the position.
     * parse_line has to catch, set the proper values and rethrow
     */
    syntax_error(string msg, string file = "", int line_no = -1)
      : parser_error(msg), file(file), line_no(line_no), msg(msg) {}

    virtual const char* what() const throw() {
      return ("Syntax error at " + file + ":" + to_string(line_no) + " " + msg).c_str();
    }

  private:
    string file;
    int line_no;
    string msg;
};

/**
 * \brief syntax_error subclass for invalid names
 */
class invalid_name_error : public syntax_error {
  public:
    /**
     * type is the type of name (Header, Key)
     */
    invalid_name_error(string type, string name)
      : syntax_error(type + " '" + name + "' is an invalid name") {}
};

/**
 * \enum line_type
 * \brief All different types a line in a config can be
 */
enum line_type {KEY, HEADER, COMMENT, EMPTY, UNKNOWN};

/**
 * \struct line_t
 * \brief Storage for a single config line
 *
 * More sanitized than the actual string of the comment line, with information
 * about line type and structure
 */
struct line_t {
  string header;
  string key_value[2];

  /**
   * Whether or not this struct contains a valid line
   * If false all other fields are not set
   * Set this to false, if you want to return a line that has no effect
   * (for example when you parse a comment line), do not use this to signal
   * any kind of error, throw an exception instead
   */
  bool is_valid;

  /**
   * Index of the config_parser::files vector where this line is from
   */
  int file_index;
  int line_no;

  /**
   * We access header, if is_header == true otherwise we access key_value
   */
  bool is_header;
};

using valuemap_t = std::unordered_map<string, string>;
using sectionmap_t = std::map<string, valuemap_t>;
using file_list = vector<string>;

/**
 * \brief Represents the whole configuration the user has given
 */
struct config_file {
  /**
   * Maps sections to a list of its key-value pairs
   */
  sectionmap_t sections;

  /**
   * Full path to the config file
   */
  string filename;

  /**
   * All files included by the config (not including itself)
   *
   */
  file_list included;
};

class config_parser {
  public:

    config_parser(const logger& logger, string&& file);

    /**
     * \brief Performs the parsing of the main config file
     *
     * Goes through multiple steps:
     * - Reads and parses all the lines in the config file while resolving
     *   include-file directives and detecting cyclic dependencies in those
     *   directives.
     * - Builds a graph. The nodes are key value pairs and (u, v) is an edge,
     *   when the value of u references v (via ${section.key} and the like)
     * - The topological order is calculated. If that is not possible, there
     *   are cyclic dependencies in the config and it is not valid. We reverse
     *   the topological order, traverse the nodes in that order and
     *   dereference all references. In the end no valid ${...} references
     *   should be left.
     * - Builds a second graph. The nodes are sections and (u, v) is and edge,
     *   iff u has an inherit key that points to v.
     * - The topological order of this second graph is calculated and all
     *   sections without an inherit key are ignored.
     * - The topological order is reversed, the sections are traversed in
     *   that order, and the keys of the inherited sections are pulled in.
     *   Here, there shouldn't be any inherit keys left.
     * - Finally the data is put into a sectionmap_t
     *
     * \returns config_file struct containing a
     *          section <-> list of key-value pair mapping for the caller to
     *          be used. All references in the value strings will already have
     *          been resolved and can be used without further processing.
     *
     * \throws syntax_error If there was any kind of syntax error
     * \throws parser_error If aynthing else went wrong
     */
    config_file parse();

  protected:

    /**
     * \brief Parses the given file and extracts key-value pairs and section
     *        headers and adds them onto the `lines` vector
     *
     * This method directly resolves `include-file` directives and checks for
     * cyclic dependencies
     *
     * It is assumed the file exists
     */
    void parse_file(string file, file_list path);

    /**
     * \brief Parses the given line string which occurs in files[file_index] on line
     *        line_no and creates a line_t struct
     *
     * We use the INI file syntax (https://en.wikipedia.org/wiki/INI_file);
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
     * \throws syntax_error if the line isn't well formed
     */
    line_t parse_line(int file_index, int line_no, string line);

    /**
     * \brief Deterimes the type of a line read from a config file
     *
     * Expects that line is trimmed
     * This only looks at the first character and doesn't check if the line is
     * actually syntactically correct.
     * HEADER ('['), COMMENT (';' or '#') and EMPTY (None) are uniquely
     * identified by their first character (or lack thereof). Any line that
     * is none of the above and contains an equal sign, is treated as KEY.
     * All others are UNKNOWN
     */
    line_type get_line_type(string line);

    /**
     * \brief Parse a line containing a section header and returns the header name
     *
     * Only assumes that the line starts with '[' and is trimmed
     *
     * \throws syntax_error if the line doesn't end with ']' or the header name
     *         contains forbidden characters
     */
    string parse_header(string line);

    /**
     * \brief Parses a line containing a key-value pair and returns the key name
     *        and the value string inside a std::pair
     *
     * Only assumes that the line contains '=' at least once and is trimmed
     *
     * \throws syntax_error if the key contains forbidden characters
     */
    std::pair<string, string> parse_key(string line);

  private:

    /**
     * \brief Checks if the given name doesn't contain any spaces or characters
     *        in config_parser::forbidden_chars
     */
    bool is_valid_name(string name);

    const logger& m_log;

    /**
     * \brief Full path to the main config file
     */
    string m_file;

    /**
     * \brief Name of all the files the config includes values from
     *
     * The line_t struct uses indices to this vector to map lines to their
     * original files. This allows us to point the user to the exact location
     * of errors
     */
    file_list files;

    /*
     * \brief List of all the lines in the config (with included files)
     *
     * The order here matters, as we have not yet associated key-value pairs
     * with sections
     */
    vector<line_t> lines;

    /*
     * \brief None of these characters can be used in the key and section names
     */
    string forbidden_chars{"\"'=;#[](){}:.$\\%"};
};

POLYBAR_NS_END

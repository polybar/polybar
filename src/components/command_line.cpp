#include "components/command_line.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

POLYBAR_NS

namespace command_line {
  /**
   * Create instance
   */
  parser::make_type parser::make(std::string scriptname, const options& opts) {
    return std::make_unique<parser>("Usage: " + std::move(scriptname) + " [OPTION]... [BAR]", opts);
  }

  /**
   * Construct parser
   */
  parser::parser(std::string synopsis, const options& opts)
      : m_synopsis(std::move(synopsis)), m_opts(opts) {}

  /**
   * Print application usage message
   */
  void parser::usage() const {
    std::cout << m_synopsis << "\n\n";

    // Find max length of flags for formatting
    size_t maxlen{0};
    for (const auto& opt : m_opts) {
      size_t len = opt.flag_long.length() + opt.flag.length() + 4; // Length of flag and long flag
      if (len > maxlen) {
        maxlen = len;
      }
    }

    // Print options with proper formatting
    for (const auto& opt : m_opts) {
      std::cout << "  " << opt.flag << ", " << opt.flag_long;

      if (!opt.token.empty()) {
        std::cout << "=" << opt.token;
      }

      std::cout << std::setw(static_cast<int>(maxlen + 4 - opt.flag_long.length())) << " "
                << opt.desc;

      if (!opt.values.empty()) {
        std::cout << " [Values: ";
        for (size_t i = 0; i < opt.values.size(); ++i) {
          std::cout << opt.values[i];
          if (i < opt.values.size() - 1) {
            std::cout << ", ";
          }
        }
        std::cout << "]";
      }

      std::cout << "\n";
    }

    std::cout << "\n";
  }

  /**
   * Process input values
   */
  void parser::process_input(const std::vector<std::string>& values) {
    for (size_t i = 0; i < values.size(); ++i) {
      parse(values[i], (i + 1 < values.size()) ? values[i + 1] : "");
    }
  }

  /**
   * Test if the passed option was provided
   */
  bool parser::has(const std::string& option) const {
    return m_optvalues.find(option) != m_optvalues.end();
  }

  /**
   * Test if a positional argument is defined at given index
   */
  bool parser::has(size_t index) const {
    return index < m_posargs.size();
  }

  /**
   * Get the value defined for given option
   */
  std::string parser::get(const std::string& opt) const {
    if (has(opt)) {
      return m_optvalues.at(opt);
    }
    return "";
  }

  /**
   * Get the positional argument at given index
   */
  std::string parser::get(size_t index) const {
    return has(index) ? m_posargs[index] : "";
  }

  /**
   * Compare option value with given string
   */
  bool parser::compare(const std::string& opt, const std::string& val) const {
    return get(opt) == val;
  }

  /**
   * Compare positional argument at given index with given string
   */
  bool parser::compare(size_t index, const std::string& val) const {
    return get(index) == val;
  }

  /**
   * Parse option value
   */
  std::string parser::parse_value(const std::string& input, const std::string& input_next, const std::vector<std::string>& values) const {
    size_t pos = input.find('=');
    std::string value = (pos == std::string::npos) ? input_next : input.substr(pos + 1);

    if (!values.empty() && std::find(values.begin(), values.end(), value) == values.end()) {
      throw std::invalid_argument("Invalid argument for option: " + input);
    }

    return value;
  }

  /**
   * Parse and validate passed arguments and flags
   */
  void parser::parse(const std::string& input, const std::string& input_next) {
    if (m_skipnext) {
      m_skipnext = false;
      return;
    }

    for (const auto& opt : m_opts) {
      if (input == opt.flag || input == opt.flag_long) {
        if (opt.token.empty()) {
          m_optvalues[opt.flag_long.substr(2)] = "";
        } else {
          std::string value = parse_value(input, input_next, opt.values);
          m_optvalues[opt.flag_long.substr(2)] = value;
          m_skipnext = value == input_next;
        }
        return;
      }
    }

    if (input[0] != '-') {
      m_posargs.push_back(input);
    } else {
      throw std::invalid_argument("Unrecognized option: " + input);
    }
  }
}  // namespace command_line

POLYBAR_NS_END

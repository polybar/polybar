#include "components/command_line.hpp"

#include <algorithm>

POLYBAR_NS

namespace command_line {
  /**
   * Create instance
   */
  parser::make_type parser::make(string&& scriptname, const options&& opts) {
    return std::make_unique<parser>("Usage: " + scriptname + " [OPTION]... [BAR]", forward<decltype(opts)>(opts));
  }

  /**
   * Construct parser
   */
  parser::parser(string&& synopsis, const options&& opts)
      : m_synopsis(forward<decltype(synopsis)>(synopsis)), m_opts(forward<decltype(opts)>(opts)) {}

  /**
   * Print application usage message
   */
  void parser::usage() const {
    printf("%s\n\n", m_synopsis.c_str());

    // get the length of the longest string in the flag column
    // which is used to align the description fields
    size_t maxlen{0};

    for (const auto& m_opt : m_opts) {
      size_t len{m_opt.flag_long.length() + m_opt.flag.length() + 4};
      maxlen = len > maxlen ? len : maxlen;
    }

    for (auto& opt : m_opts) {
      size_t pad = maxlen - opt.flag_long.length() - opt.token.length();

      printf("  %s, %s", opt.flag.c_str(), opt.flag_long.c_str());

      if (!opt.token.empty()) {
        printf("=%s", opt.token.c_str());
        pad--;
      }

      // output the list with accepted values
      if (!opt.values.empty()) {
        printf("%*s\n", static_cast<int>(pad + opt.desc.length()), opt.desc.c_str());

        pad = pad + opt.flag_long.length() + opt.token.length() + 7;

        printf("%*c%s is one of: ", static_cast<int>(pad), ' ', opt.token.c_str());

        for (auto& v : opt.values) {
          printf("%s%s", v.c_str(), v != opt.values.back() ? ", " : "");
        }
      } else {
        printf("%*s", static_cast<int>(pad + opt.desc.length()), opt.desc.c_str());
      }

      printf("\n");
    }

    printf("\n");
  }

  /**
   * Process input values
   */
  void parser::process_input(const vector<string>& values) {
    for (size_t i = 0; i < values.size(); i++) {
      parse(values[i], values.size() > i + 1 ? values[i + 1] : "");
    }
  }

  /**
   * Test if the passed option was provided
   */
  bool parser::has(const string& option) const {
    return m_optvalues.find(option) != m_optvalues.end();
  }

  /**
   * Test if a positional argument is defined at given index
   */
  bool parser::has(size_t index) const {
    return m_posargs.size() > index;
  }

  /**
   * Get the value defined for given option
   */
  string parser::get(string opt) const {
    if (has(forward<string>(opt))) {
      return m_optvalues.find(opt)->second;
    }
    return "";
  }

  /**
   * Get the positional argument at given index
   */
  string parser::get(size_t index) const {
    return has(index) ? m_posargs[index] : "";
  }

  /**
   * Compare option value with given string
   */
  bool parser::compare(string opt, const string& val) const {
    return get(move(opt)) == val;
  }

  /**
   * Compare positional argument at given index with given string
   */
  bool parser::compare(size_t index, const string& val) const {
    return get(index) == val;
  }

  /**
   * Compare option with its short version
   */
  auto parser::is_short(const string& option, const string& opt_short) const {
    return option.compare(0, opt_short.length(), opt_short) == 0;
  }

  /**
   * Compare option with its long version
   */
  auto parser::is_long(const string& option, const string& opt_long) const {
    return option.compare(0, opt_long.length(), opt_long) == 0;
  }

  /**
   * Compare option with both versions
   */
  auto parser::is(const string& option, string opt_short, string opt_long) const {
    return is_short(option, move(opt_short)) || is_long(option, move(opt_long));
  }

  /**
   * Parse option value
   */
  auto parser::parse_value(string input, const string& input_next, choices values) const {
    string opt = move(input);
    size_t pos;
    string value;

    if (input_next.empty() && opt.compare(0, 2, "--") != 0) {
      throw value_error("Missing argument for option " + opt);
    } else if ((pos = opt.find('=')) == string::npos && opt.compare(0, 2, "--") == 0) {
      throw value_error("Missing argument for option " + opt);
    } else if (pos == string::npos && !input_next.empty()) {
      value = input_next;
    } else {
      value = opt.substr(pos + 1);
      opt = opt.substr(0, pos);
    }

    if (!values.empty() && std::find(values.begin(), values.end(), value) == values.end()) {
      throw value_error("Invalid argument for option " + opt);
    }

    return value;
  }

  /**
   * Parse and validate passed arguments and flags
   */
  void parser::parse(const string& input, const string& input_next) {
    auto skipped = m_skipnext;
    if (m_skipnext) {
      m_skipnext = false;
      if (!input_next.empty()) {
        return;
      }
    }

    for (auto&& opt : m_opts) {
      if (is(input, opt.flag, opt.flag_long)) {
        if (opt.token.empty()) {
          m_optvalues.insert(make_pair(opt.flag_long.substr(2), ""));
        } else {
          auto value = parse_value(input, input_next, opt.values);
          m_skipnext = (value == input_next);
          m_optvalues.insert(make_pair(opt.flag_long.substr(2), value));
        }
        return;
      }
    }

    if (skipped) {
      return;
    } else if (input[0] != '-') {
      m_posargs.emplace_back(input);
    } else {
      throw argument_error("Unrecognized option " + input);
    }
  }
}  // namespace command_line

POLYBAR_NS_END

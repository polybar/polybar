#include <iomanip>
#include <iostream>

#include "components/command_line.hpp"

LEMONBUDDY_NS

/**
 * Print application usage message
 */
void cliparser::usage() const {
  std::cout << m_synopsis << "\n" << std::endl;

  // get the length of the longest string in the flag column
  // which is used to align the description fields
  size_t maxlen{0};

  for (auto it = m_opts.begin(); it != m_opts.end(); ++it) {
    size_t len{it->flag_long.length() + it->flag.length() + 4};
    maxlen = len > maxlen ? len : maxlen;
  }

  for (auto& opt : m_opts) {
    size_t pad = maxlen - opt.flag_long.length() - opt.token.length();

    std::cout << "  " << opt.flag << ", " << opt.flag_long;

    if (!opt.token.empty()) {
      std::cout << "=" << opt.token;
      pad--;
    }

    // output the list with accepted values
    if (!opt.values.empty()) {
      std::cout << std::setw(pad + opt.desc.length()) << std::setfill(' ') << opt.desc << std::endl;

      pad = pad + opt.flag_long.length() + opt.token.length() + 7;

      std::cout << string(pad, ' ') << opt.token << " is one of: ";

      for (auto& v : opt.values) {
        std::cout << v << (v != opt.values.back() ? ", " : "");
      }
    } else {
      std::cout << std::setw(pad + opt.desc.length()) << std::setfill(' ') << opt.desc;
    }

    std::cout << std::endl;
  }
}

/**
 * Process input values
 *
 * This is done outside the constructor due to boost::di noexcept
 */
void cliparser::process_input(const vector<string>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    parse(values[i], values.size() > i + 1 ? values[i + 1] : "");
  }
}

/**
 * Test if the passed option was provided
 */
bool cliparser::has(const string& option) const {
  return m_optvalues.find(option) != m_optvalues.end();
}

/**
 * Gets the value defined for given option
 */
string cliparser::get(string opt) const {
  if (has(forward<string>(opt)))
    return m_optvalues.find(opt)->second;
  return "";
}

/**
 * Compare option value with given string
 */
bool cliparser::compare(string opt, string val) const {
  return get(opt) == val;
}

/**
 * Compare option with its short version
 */
auto cliparser::is_short(string option, string opt_short) const {
  return option.compare(0, opt_short.length(), opt_short) == 0;
}

/**
 * Compare option with its long version
 */
auto cliparser::is_long(string option, string opt_long) const {
  return option.compare(0, opt_long.length(), opt_long) == 0;
}

/**
 * Compare option with both versions
 */
auto cliparser::is(string option, string opt_short, string opt_long) const {
  return is_short(option, opt_short) || is_long(option, opt_long);
}

/**
 * Parse option value
 */
auto cliparser::parse_value(string input, string input_next, choices values) const {
  string opt = input;
  size_t pos;
  string value;

  if (input_next.empty() && opt.compare(0, 2, "--") != 0)
    throw value_error("Missing value for " + opt);
  else if ((pos = opt.find("=")) == string::npos && opt.compare(0, 2, "--") == 0)
    throw value_error("Missing value for " + opt);
  else if (pos == string::npos && !input_next.empty())
    value = input_next;
  else {
    value = opt.substr(pos + 1);
    opt = opt.substr(0, pos);
  }

  if (!values.empty() && std::find(values.begin(), values.end(), value) == values.end())
    throw value_error("Invalid value '" + value + "' for argument " + string{opt});

  return value;
}

/**
 * Parse and validate passed arguments and flags
 */
void cliparser::parse(string input, string input_next) {
  if (m_skipnext) {
    m_skipnext = false;
    if (!input_next.empty())
      return;
  }

  for (auto&& opt : m_opts) {
    if (is(input, opt.flag, opt.flag_long)) {
      if (opt.token.empty()) {
        m_optvalues.insert(std::make_pair(opt.flag_long.substr(2), ""));
      } else {
        auto value = parse_value(input, input_next, opt.values);
        m_skipnext = (value == input_next);
        m_optvalues.insert(std::make_pair(opt.flag_long.substr(2), value));
      }

      return;
    }
  }

  if (input.compare(0, 1, "-") == 0) {
    throw argument_error("Unrecognized option " + input);
  }
}

LEMONBUDDY_NS_END

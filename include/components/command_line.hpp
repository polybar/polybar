#pragma once

#include <map>

#include "common.hpp"
#include "errors.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

namespace command_line {
  DEFINE_ERROR(argument_error);
  DEFINE_ERROR(value_error);

  class option;
  using choices = vector<string>;
  using options = vector<option>;
  using values = std::map<string, string>;

  // class definition : option {{{

  class option {
   public:
    string flag;
    string flag_long;
    string desc;
    string token;
    const choices values;

    explicit option(string flag, string flag_long, string desc, string token = "", const choices c = {})
        : flag(flag), flag_long(flag_long), desc(desc), token(token), values(c) {}
  };

  // }}}
  // class definition : parser {{{

  class parser {
   public:
    static unique_ptr<parser> make(string scriptname, const options& opts);

    explicit parser(const string& synopsis, const options& opts) : m_synopsis(synopsis), m_opts(opts) {}

    void usage() const;

    void process_input(const vector<string>& values);

    bool has(const string& option) const;
    string get(string opt) const;
    bool compare(string opt, const string& val) const;

   protected:
    auto is_short(const string& option, const string& opt_short) const;
    auto is_long(const string& option, const string& opt_long) const;
    auto is(const string& option, string opt_short, string opt_long) const;

    auto parse_value(string input, const string& input_next, choices values) const;
    void parse(const string& input, const string& input_next = "");

   private:
    string m_synopsis;
    options m_opts;
    values m_optvalues;
    bool m_skipnext = false;
  };

  // }}}
}

using cliparser = command_line::parser;
using clioption = command_line::option;
using clioptions = command_line::options;

POLYBAR_NS_END

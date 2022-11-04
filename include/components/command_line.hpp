#pragma once

#include <map>

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

namespace command_line {
  DEFINE_ERROR(argument_error);
  DEFINE_ERROR(value_error);

  class option;
  using choices = vector<string>;
  using options = vector<option>;
  using values = std::map<string, string>;
  using posargs = vector<string>;

  // class definition : option {{{

  class option {
   public:
    string flag;
    string flag_long;
    string desc;
    string token;
    const choices values;

    explicit option(string&& flag, string&& flag_long, string&& desc, string&& token = "", const choices&& c = {})
        : flag(forward<string>(flag))
        , flag_long(forward<string>(flag_long))
        , desc(forward<string>(desc))
        , token(forward<string>(token))
        , values(forward<const choices>(c)) {}
  };

  // }}}
  // class definition : parser {{{

  class parser {
   public:
    using make_type = unique_ptr<parser>;
    static make_type make(string&& scriptname, const options&& opts);

    explicit parser(string&& synopsis, const options&& opts);

    void usage() const;

    void process_input(const vector<string>& values);

    bool has(const string& option) const;
    bool has(size_t index) const;
    string get(string opt) const;
    string get(size_t index) const;
    bool compare(string opt, const string& val) const;
    bool compare(size_t index, const string& val) const;

   protected:
    auto is_short(const string& option, const string& opt_short) const;
    auto is_long(const string& option, const string& opt_long) const;
    auto is(const string& option, string opt_short, string opt_long) const;

    auto parse_value(string input, const string& input_next, choices values) const;
    void parse(const string& input, const string& input_next = "");

   private:
    string m_synopsis{};
    const options m_opts;
    values m_optvalues{};
    posargs m_posargs{};
    bool m_skipnext{false};
  };

  // }}}
}

POLYBAR_NS_END

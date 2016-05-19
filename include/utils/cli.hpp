#ifndef _UTILS_CLI_HPP_
#define _UTILS_CLI_HPP_

#include <string>
#include <vector>

#include "exception.hpp"

namespace cli
{
  class CommandLineError : public Exception {
    using Exception::Exception;
  };

  struct Option
  {
    std::string flag_short;
    std::string flag_long;
    std::string placeholder;
    std::string help;
    std::vector<std::string> accept;

    Option(const std::string& flag_short, const std::string& flag_long, const std::string& placeholder, std::vector<std::string> accept, std::string help)
      : flag_short(flag_short), flag_long(flag_long), placeholder(placeholder), help(help), accept(accept){}
  };

  void add_option(const std::string& opt_short, const std::string& opt_long, const std::string& help);
  void add_option(const std::string& opt_short, const std::string& opt_long, const std::string& placeholder, const std::string& help, std::vector<std::string> accept = {});

  bool is_option(char *opt, const std::string& opt_short, const std::string& opt_long);
  bool is_option_short(char *opt, const std::string& opt_short);
  bool is_option_long(char *opt, const std::string& opt_long);

  std::string parse_option_value(int &i, int argc, char **argv, std::vector<std::string> accept = {});
  bool has_option(const std::string& opt);
  std::string get_option_value(const std::string& opt);
  bool match_option_value(const std::string& opt, const std::string& val);

  void parse(int i, int argc, char **argv);

  void usage(const std::string& usage, bool exit_success = true);
}

#endif

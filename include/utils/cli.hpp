#pragma once

#include <vector>

#include "exception.hpp"

namespace cli
{
  DefineBaseException(CommandLineError);

  struct Option
  {
    std::string flag_short;
    std::string flag_long;
    std::string placeholder;
    std::string help;
    std::vector<std::string> accept;

    Option(std::string flag_short, std::string flag_long, std::string placeholder, std::vector<std::string> accept, std::string help)
      : flag_short(flag_short), flag_long(flag_long), placeholder(placeholder), help(help), accept(accept){}
  };

  void add_option(std::string opt_short, std::string opt_long, std::string help);
  void add_option(std::string opt_short, std::string opt_long, std::string placeholder, std::string help, std::vector<std::string> accept = {});

  bool is_option(char *opt, std::string opt_short, std::string opt_long);
  bool is_option_short(char *opt, std::string opt_short);
  bool is_option_long(char *opt, std::string opt_long);

  std::string parse_option_value(int &i, int argc, char **argv, std::vector<std::string> accept = {});
  bool has_option(std::string opt);
  std::string get_option_value(std::string opt);
  bool match_option_value(std::string opt, std::string val);

  void parse(int i, int argc, char **argv);

  void usage(std::string usage, bool exit_success = true);
}

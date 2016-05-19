#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>

#include "services/logger.hpp"
#include "utils/cli.hpp"
#include "utils/memory.hpp"

namespace cli
{
  std::vector<Option> options;
  std::map<std::string, std::string> values;

  void add_option(const std::string& opt_short, const std::string& opt_long, const std::string& help) {
    add_option(opt_short, opt_long, "", help);
  }

  void add_option(const std::string& opt_short, const std::string& opt_long, const std::string& placeholder, const std::string& help, std::vector<std::string> accept) {
    options.emplace_back(Option(opt_short, opt_long, placeholder, accept, help));
  }

  bool is_option(char *opt, const std::string& opt_short, const std::string& opt_long) {
    return is_option_short(opt, opt_short) || is_option_long(opt, opt_long);
  }

  bool is_option_short(char *opt, const std::string& opt_short) {
    return std::strncmp(opt, opt_short.c_str(), opt_short.length()) ==  0;
  }

  bool is_option_long(char *opt, const std::string& opt_long) {
    return std::strncmp(opt, opt_long.c_str(), opt_long.length()) == 0;
  }

  std::string parse_option_value(int &i, int argc, char **argv, std::vector<std::string> accept)
  {
    std::string value, opt = std::string(argv[i]);

    if (opt.find("--", 0, 2) != std::string::npos) {
      auto pos = opt.find("=");
      if (pos == std::string::npos) {
        throw CommandLineError("Missing value for "+ opt);
      }
      value = opt.substr(pos + 1);
      opt = opt.substr(0, pos);
    } else if (i < argc - 1) {
      value = std::string(argv[++i]);
    } else throw CommandLineError("Missing value for "+ opt);

    if (!accept.empty() && std::find(accept.begin(), accept.end(), value) == accept.end())
      throw CommandLineError("Invalid argument '"+ value +"' for "+ std::string(opt));
    return value;
  }

  bool has_option(const std::string& opt) {
    return values.find(opt) != values.end();
  }

  std::string get_option_value(const std::string& opt)
  {
    if (has_option(opt))
      return values.find(opt)->second;
    else return "";
  }

  bool match_option_value(const std::string& opt, const std::string& val) {
    return get_option_value(opt) == val;
  }

  void parse(int i, int argc, char **argv)
  {
    for (; i < argc; i++) {
      Option *opt = nullptr;

      for (auto &o : options) {
        if (is_option(argv[i], o.flag_short, o.flag_long)) {
          opt = &o;
          break;
        }
      }
      if (opt == nullptr) throw CommandLineError("Unrecognized argument: "+ std::string(argv[i]));
      if (opt->placeholder.empty())
        values.insert(std::make_pair(opt->flag_long.substr(2), ""));
      else
        values.insert(std::make_pair(opt->flag_long.substr(2), parse_option_value(i, argc, argv, opt->accept)));
    }
  }

  void usage(const std::string& usage, bool exit_success)
  {
    int longest_n = 0, n;

    for (auto &o : options) {
      if ((n = o.flag_long.length() + o.placeholder.length() + 1) > longest_n)
        longest_n = n;
    }

    std::cout << usage << std::endl;
    std::cout << "" << std::endl;

    for (auto &o : options) {
      std:: string spacing(longest_n - o.flag_long.length() - o.placeholder.length() + 3, ' ');
      std::cout << "  " << o.flag_short;
      std::cout << ", " << o.flag_long;

      if (!o.placeholder.empty()) {
        std::cout << "=" << o.placeholder;
        spacing.erase(0, 1);
      }

      std::cout << spacing << o.help << std::endl;

      if (!o.accept.empty()) {
        spacing.append(std::string(o.flag_long.length() + o.placeholder.length() + 7, ' '));
        std::cout << spacing << o.placeholder << " is one of: ";
        for (auto &a : o.accept)
          std::cout << a << (a != o.accept.back() ? ", " : "");
        std::cout << std::endl;
      }
    }

    std::cout << "" << std::endl;

    std::exit(exit_success ? EXIT_SUCCESS : EXIT_FAILURE);
  }
}

#include "components/config_parser.hpp"

POLYBAR_NS

config_parser::config_parser(const logger& logger, string&& file)
  : m_log(logger), m_file(forward<string>(file)) {

    if (!file_util::exists(m_file)) {
      throw application_error("Could not find config file: " + m_file);
    }

    m_log.trace("Parsing config file: %s", m_file);

  }

config_file parse() {
  return {};
}

void config_parser::parse_file(string file, file_list path) {
}

line_t config_parser::parse_line(int file_index, int line_no, string line) {
  return {};
}

line_type config_parser::get_line_type(string line) {
  return line_type::UNKNOWN;
}

string config_parser::parse_header(string line) {
  return "";
}

std::pair<string, string> config_parser::parse_key(string line) {
  return {"", ""};
}

bool config_parser::is_valid_name(string name) {
  return false;
}

POLYBAR_NS_END

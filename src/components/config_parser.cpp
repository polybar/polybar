#include <fstream>

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
  if(std::find(path.begin(), path.end(), file) != path.end()) {
    // We have already parsed this file in this path, so there are cyclic dependencies
    // TODO print dependency cycle
    throw application_error("Cyclic dependency detected while parsing " + file);
  }

  int file_index = files.size();
  files.push_back(file);
  path.push_back(file);

  int line_no = 0;

  string line_str;

  // TODO error handling
  std::ifstream in(file);
  while (std::getline(in, line_str)) {
    line_no++;
    line_t line;
    try {
      line = parse_line(line_str);

      // parse_line doesn't set these
      line.file_index = file_index;
      line.line_no = line_no;
    } catch(syntax_error& err) {
      /*
       * Exceptions thrown by parse_line doesn't have the line
       * numbers and files set, so we have to add them here
       */
      throw syntax_error(err.get_msg(), files[file_index], line_no);
    }

    // Not valid means 'not important to us' and never implies a syntax error
    if(!line.is_valid) {
      continue;
    }

    if(!line.is_header && line.key_value[0] == "include-file") {
      file_list cloned_path(path);

      parse_file(line.key_value[1], cloned_path);
    }
    else {
      lines.push_back(line);
    }

  }
}

line_t config_parser::parse_line(string line) {
  line = string_util::trim(line, string_util::isspace_pred);
  line_type type = get_line_type(line);

  line_t result = {};

  if(type == EMPTY || type == COMMENT) {
    result.is_valid = false;
    return result;
  }

  if(type == UNKNOWN) {
    throw syntax_error("Unknown line type " + line);
  }

  result.is_valid = true;

  if(type == HEADER) {
    result.is_header = true;
    result.header = parse_header(line);
  }

  if(type == KEY) {
    result.is_header = false;
    auto key_value = parse_key(line);
    result.key_value[0] = key_value.first;
    result.key_value[1] = key_value.second;
  }

  return result;
}

line_type config_parser::get_line_type(string line) {
  if(line.empty()) {
    return EMPTY;
  }

  switch (line[0]) {
    case '[':
      return HEADER;

    case ';':
    case '#':
      return COMMENT;

    default:
      if(string_util::contains(line, "=")) {
        return KEY;
      }
      else {
        return UNKNOWN;
      }
  }
}

string config_parser::parse_header(string line) {
  if(line.back() != ']') {
    throw syntax_error("Missing ']' in header '" + line + "'");
  }

  string header = line.substr(1, line.size() - 2);

  if(!is_valid_name(header)) {
    throw invalid_name_error("Header", header);
  }

  return header;
}

std::pair<string, string> config_parser::parse_key(string line) {
  size_t pos = line.find_first_of('=');

  string key = trim(line.substr(0, pos), string_util::isspace_pred);
  string value = trim(line.substr(pos + 1), string_util::isspace_pred);

  if(!is_valid_name(key)) {
    throw invalid_name_error("Key", key);
  }

  /*
   * Remove double quotes around value, only if it starts end ends with
   * double quotes
   */
  if(value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }

  return {key, value};
}

bool config_parser::is_valid_name(string name) {
  if(name.empty()) {
    return false;
  }

  for (size_t i = 0; i < name.size(); i++) {
    char c = name[i];
    // Names with forbidden chars or spaces are not valid
    if(isspace(c) || forbidden_chars.find_first_of(c) != string::npos) {
        return false;
      }
  }

  return true;
}

POLYBAR_NS_END

#include <fstream>
#include <algorithm>

#include "components/config_parser.hpp"

POLYBAR_NS

config_parser::config_parser(const logger& logger, const string&& file, const string&& bar)
  : m_log(logger),
  m_file(file_util::expand(forward<const string>(file))),
  m_barname(forward<const string>(bar)) {}

config::make_type config_parser::parse() {
  m_log.info("Parsing config file: %s", m_file);

  parse_file(m_file, {});

  sectionmap_t sections = create_sectionmap();

  if(sections.find("bar/" + m_barname) == sections.end()) {
    throw application_error("Undefined bar: " + m_barname);
  }

  /*
   * The first element in the files vector is always the main config file and
   * because it has unique filenames, we can use all the elements from the
   * second element onwards for the included list
   */
  file_list included(files.begin() + 1, files.end());
  config::make_type result = config::make(m_file, m_barname);

  // Cast to non-const to set sections, included and xrm
  config& m_conf = const_cast<config&>(result);

  m_conf.set_sections(sections);
  m_conf.set_included(included);
  if(use_xrm) {
    m_conf.use_xrm();
  }

  return result;
}

sectionmap_t config_parser::create_sectionmap() {
  sectionmap_t sections{};

  string current_section = "";

  for(line_t line : lines) {
    if(!line.useful) {
      continue;
    }

    if(line.is_header) {
      current_section = line.header;
    } else {
      // The first valid line in the config is not a section definition
      if(current_section == "") {
        throw syntax_error("First valid line in config must be section header",
            files[line.file_index], line.line_no);
      }

      string key = line.key;
      string value = line.value;

      valuemap_t& valuemap = sections[current_section];

      if(valuemap.find(key) == valuemap.end()) {
        valuemap.emplace(key, value);
      } else {
        // Key already exists in this section
        throw syntax_error("Duplicate key name \"" + key
            + "\" defined in section \"" + current_section + "\"",
            files[line.file_index], line.line_no);
      }
    }
  }

  return sections;
}

void config_parser::parse_file(const string& file, file_list path) {
  if(std::find(path.begin(), path.end(), file) != path.end()) {
    string path_str{};

    for (auto p : path) {
      path_str += ">\t" + p + "\n";
    }

    path_str += ">\t" + file;

    // We have already parsed this file in this path, so there are cyclic dependencies
    throw application_error("include-file: Dependency cycle detected:\n" + path_str);
  }

  m_log.trace("config_parser: Parsing %s", file);

  int file_index;

  auto found = std::find(files.begin(), files.end(), file);

  if(found == files.end()) {
    file_index = files.size();
    files.push_back(file);
  } else {
    /*
     * `file` is already in the `files` vector so we calculate its index.
     *
     * This means that the file was already parsed, this can happen without
     * cyclic dependencies, if the file is included twice
     */
    file_index = found - files.begin();
  }

  path.push_back(file);

  int line_no = 0;

  string line_str;

  std::ifstream in(file);

  if (!in) {
    throw application_error("Failed to open config file " + file + ": " + strerror(errno));
  }

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

    // Skip useless lines (comments, empty lines)
    if(!line.useful) {
      continue;
    }

    if(!line.is_header && line.key == "include-file") {
      parse_file(file_util::expand(line.value), path);
    } else {
      lines.push_back(line);
    }
  }
}

line_t config_parser::parse_line(const string& line) {
  string line_trimmed = string_util::trim(line, string_util::isspace_pred);
  line_type type = get_line_type(line_trimmed);

  line_t result = {};

  if(type == EMPTY || type == COMMENT) {
    result.useful = false;
    return result;
  }

  if(type == UNKNOWN) {
    throw syntax_error("Unknown line type: " + line_trimmed);
  }

  result.useful = true;

  if(type == HEADER) {
    result.is_header = true;
    result.header = parse_header(line_trimmed);
  } else if(type == KEY) {
    result.is_header = false;
    auto key_value = parse_key(line_trimmed);
    result.key = key_value.first;
    result.value = key_value.second;
  }

  return result;
}

line_type config_parser::get_line_type(const string& line) {
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
      } else {
        return UNKNOWN;
      }
  }
}

string config_parser::parse_header(const string& line) {
  if(line.back() != ']') {
    throw syntax_error("Missing ']' in header '" + line + "'");
  }

  // Stripping square brackets
  string header = line.substr(1, line.size() - 2);

  if(!is_valid_name(header)) {
    throw invalid_name_error("Section", header);
  }

  if(reserved_section_names.find(header) != reserved_section_names.end()) {
    throw syntax_error("'" + header + "' is reserved and cannot be used as a section name");
  }

  return header;
}

std::pair<string, string> config_parser::parse_key(const string& line) {
  size_t pos = line.find_first_of('=');

  string key = trim(line.substr(0, pos), string_util::isspace_pred);
  string value = trim(line.substr(pos + 1), string_util::isspace_pred);

  if(!is_valid_name(key)) {
    throw invalid_name_error("Key", key);
  }

  /*
   * Only if the string is surrounded with double quotes, do we treat them
   * not as part of the value and remove them.
   */
  if(value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }

  // TODO check value for references

#if WITH_XRM
  // Use xrm, if at least one value is an xrdb reference
  if(!use_xrm && value.find("${xrdb") == 0) {
    use_xrm = true;
  }
#endif

  return {key, value};
}

bool config_parser::is_valid_name(const string& name) {
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

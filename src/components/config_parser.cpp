#include "components/config_parser.hpp"

#include <algorithm>
#include <fstream>

POLYBAR_NS

config_parser::config_parser(const logger& logger, string&& file, string&& bar)
    : m_log(logger), m_config(file_util::expand(file)), m_barname(move(bar)) {}

config::make_type config_parser::parse() {
  m_log.notice("Parsing config file: %s", m_config);

  parse_file(m_config, {});

  sectionmap_t sections = create_sectionmap();

  if (sections.find("bar/" + m_barname) == sections.end()) {
    throw application_error("Undefined bar: " + m_barname);
  }

  /*
   * The first element in the files vector is always the main config file and
   * because it has unique filenames, we can use all the elements from the
   * second element onwards for the included list
   */
  file_list included(m_files.begin() + 1, m_files.end());
  config::make_type result = config::make(m_config, m_barname);

  // Cast to non-const to set sections, included and xrm
  config& m_conf = const_cast<config&>(result);

  m_conf.set_sections(move(sections));
  m_conf.set_included(move(included));
  if (use_xrm) {
    m_conf.use_xrm();
  }

  return result;
}

sectionmap_t config_parser::create_sectionmap() {
  sectionmap_t sections{};

  string current_section{};

  for (const line_t& line : m_lines) {
    if (!line.useful) {
      continue;
    }

    if (line.is_header) {
      current_section = line.header;
    } else {
      // The first valid line in the config is not a section definition
      if (current_section.empty()) {
        throw syntax_error("First valid line in config must be section header", m_files[line.file_index], line.line_no);
      }

      const string& key = line.key;
      const string& value = line.value;

      valuemap_t& valuemap = sections[current_section];

      if (valuemap.find(key) == valuemap.end()) {
        valuemap.emplace(key, value);
      } else {
        // Key already exists in this section
        throw syntax_error("Duplicate key name \"" + key + "\" defined in section \"" + current_section + "\"",
            m_files[line.file_index], line.line_no);
      }
    }
  }

  return sections;
}

void config_parser::parse_file(const string& file, file_list path) {
  if (std::find(path.begin(), path.end(), file) != path.end()) {
    string path_str{};

    for (const auto& p : path) {
      path_str += ">\t" + p + "\n";
    }

    path_str += ">\t" + file;

    // We have already parsed this file in this path, so there are cyclic dependencies
    throw application_error("include-file: Dependency cycle detected:\n" + path_str);
  }

  m_log.trace("config_parser: Parsing %s", file);

  int file_index;

  auto found = std::find(m_files.begin(), m_files.end(), file);

  if (found == m_files.end()) {
    file_index = m_files.size();
    m_files.push_back(file);
  } else {
    /*
     * `file` is already in the `files` vector so we calculate its index.
     *
     * This means that the file was already parsed, this can happen without
     * cyclic dependencies, if the file is included twice
     */
    file_index = found - m_files.begin();
  }

  path.push_back(file);

  int line_no = 0;

  string line_str{};

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
    } catch (syntax_error& err) {
      /*
       * Exceptions thrown by parse_line doesn't have the line
       * numbers and files set, so we have to add them here
       */
      throw syntax_error(err.get_msg(), m_files[file_index], line_no);
    }

    // Skip useless lines (comments, empty lines)
    if (!line.useful) {
      continue;
    }

    if (!line.is_header && line.key == "include-file") {
      parse_file(file_util::expand(line.value), path);
    } else {
      m_lines.push_back(line);
    }
  }
}

line_t config_parser::parse_line(const string& line) {
  if (string_util::contains(line, "\ufeff")) {
    throw syntax_error(
        "This config file uses UTF-8 with BOM, which is not supported. Please use plain UTF-8 without BOM.");
  }

  string line_trimmed = string_util::trim(line, isspace);
  line_type type = get_line_type(line_trimmed);

  line_t result = {};

  if (type == line_type::EMPTY || type == line_type::COMMENT) {
    result.useful = false;
    return result;
  }

  if (type == line_type::UNKNOWN) {
    throw syntax_error("Unknown line type: " + line_trimmed);
  }

  result.useful = true;

  if (type == line_type::HEADER) {
    result.is_header = true;
    result.header = parse_header(line_trimmed);
  } else if (type == line_type::KEY) {
    result.is_header = false;
    auto key_value = parse_key(line_trimmed);
    result.key = key_value.first;
    result.value = key_value.second;
  }

  return result;
}

line_type config_parser::get_line_type(const string& line) {
  if (line.empty()) {
    return line_type::EMPTY;
  }

  switch (line[0]) {
    case '[':
      return line_type::HEADER;

    case ';':
    case '#':
      return line_type::COMMENT;

    default: {
      if (string_util::contains(line, "=")) {
        return line_type::KEY;
      } else {
        return line_type::UNKNOWN;
      }
    }
  }
}

string config_parser::parse_header(const string& line) {
  if (line.back() != ']') {
    throw syntax_error("Missing ']' in header '" + line + "'");
  }

  // Stripping square brackets
  string header = line.substr(1, line.size() - 2);

  if (!is_valid_name(header)) {
    throw invalid_name_error("Section", header);
  }

  if (m_reserved_section_names.find(header) != m_reserved_section_names.end()) {
    throw syntax_error("'" + header + "' is reserved and cannot be used as a section name");
  }

  return header;
}

std::pair<string, string> config_parser::parse_key(const string& line) {
  size_t pos = line.find_first_of('=');

  string key = string_util::trim(line.substr(0, pos), isspace);
  string value = string_util::trim(line.substr(pos + 1), isspace);

  if (!is_valid_name(key)) {
    throw invalid_name_error("Key", key);
  }

  /*
   * Only if the string is surrounded with double quotes, do we treat them
   * not as part of the value and remove them.
   */
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }

  // TODO check value for references

#if WITH_XRM
  // Use xrm, if at least one value is an xrdb reference
  if (!use_xrm && value.find("${xrdb") == 0) {
    use_xrm = true;
  }
#endif

  return {move(key), move(value)};
}

bool config_parser::is_valid_name(const string& name) {
  if (name.empty()) {
    return false;
  }

  for (const char c : name) {
    // Names with forbidden chars or spaces are not valid
    if (isspace(c) || m_forbidden_chars.find_first_of(c) != string::npos) {
      return false;
    }
  }

  return true;
}

POLYBAR_NS_END

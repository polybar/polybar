#include "components/config_parser.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>

#include "utils/file.hpp"
#include "utils/string.hpp"

POLYBAR_NS

config_parser::config_parser(const logger& logger, string&& file)
    : m_log(logger), m_config_file(file_util::expand(file)) {}

config config_parser::parse(string barname) {
  m_log.notice("Parsing config file: %s", m_config_file);

  parse_file(m_config_file, {});

  sectionmap_t sections = create_sectionmap();

  vector<string> bars = get_bars(sections);
  if (barname.empty()) {
    if (bars.size() == 1) {
      barname = bars[0];
    } else if (bars.empty()) {
      throw application_error("The config file contains no bar.");
    } else {
      throw application_error("The config file contains multiple bars, but no bar name was given. Available bars: " +
                              string_util::join(bars, ", "));
    }
  } else if (sections.find("bar/" + barname) == sections.end()) {
    if (bars.empty()) {
      throw application_error("Undefined bar: " + barname + ". The config file contains no bar.");
    } else {
      throw application_error(
          "Undefined bar: " + barname + ". Available bars: " + string_util::join(get_bars(sections), ", "));
    }
  }

  /*
   * The first element in the files vector is always the main config file and
   * because it has unique filenames, we can use all the elements from the
   * second element onwards for the included list
   */
  file_list included(m_files.begin() + 1, m_files.end());
  config conf(m_log, move(m_config_file), move(barname));

  conf.set_sections(move(sections));
  conf.set_included(move(included));
  if (use_xrm) {
    conf.use_xrm();
  }

  return conf;
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

/**
 * Get the bars declared
 */
vector<string> config_parser::get_bars(const sectionmap_t& sections) const {
  vector<string> bars;
  for (const auto& it : sections) {
    if (it.first.find(config::BAR_PREFIX) == 0) {
      bars.push_back(it.first.substr(strlen(config::BAR_PREFIX)));
    }
  }
  return bars;
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

  if (!file_util::exists(file)) {
    throw application_error("Failed to open config file " + file + ": " + strerror(errno));
  }

  if (file_util::is_dir(file)) {
    throw application_error("Config file " + file + " is a directory");
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

  auto dirname = file_util::dirname(file);

  while (std::getline(in, line_str)) {
    line_no++;
    line_t line;
    line.file_index = file_index;
    line.line_no = line_no;
    parse_line(line, line_str);

    // Skip useless lines (comments, empty lines)
    if (!line.useful) {
      continue;
    }

    if (!line.is_header && line.key == "include-file") {
      parse_file(file_util::expand(line.value, dirname), path);
    } else if (!line.is_header && line.key == "include-directory") {
      const string expanded_path = file_util::expand(line.value, dirname);
      vector<string> file_list = file_util::list_files(expanded_path);
      sort(file_list.begin(), file_list.end());
      for (const auto& filename : file_list) {
        parse_file(expanded_path + "/" + filename, path);
      }
    } else {
      m_lines.push_back(line);
    }
  }
}

void config_parser::parse_line(line_t& line, const string& line_str) {
  if (string_util::contains(line_str, "\ufeff")) {
    throw syntax_error(
        "This config file uses UTF-8 with BOM, which is not supported. Please use plain UTF-8 without BOM.",
        m_files[line.file_index], line.line_no);
  }

  string line_trimmed = string_util::trim(line_str, isspace);
  line_type type = get_line_type(line_trimmed);

  if (type == line_type::EMPTY || type == line_type::COMMENT) {
    line.useful = false;
    return;
  }

  if (type == line_type::UNKNOWN) {
    throw syntax_error("Unknown line type: " + line_trimmed, m_files[line.file_index], line.line_no);
  }

  line.useful = true;

  if (type == line_type::HEADER) {
    line.is_header = true;
    line.header = parse_header(line, line_trimmed);
  } else if (type == line_type::KEY) {
    line.is_header = false;
    auto key_value = parse_key(line, line_trimmed);
    line.key = key_value.first;
    line.value = key_value.second;
  }
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

string config_parser::parse_header(const line_t& line, const string& line_str) {
  if (line_str.back() != ']') {
    throw syntax_error("Missing ']' in header '" + line_str + "'", m_files[line.file_index], line.line_no);
  }

  // Stripping square brackets
  string header = line_str.substr(1, line_str.size() - 2);

  if (!is_valid_name(header)) {
    throw invalid_name_error("Section", header, m_files[line.file_index], line.line_no);
  }

  if (m_reserved_section_names.find(header) != m_reserved_section_names.end()) {
    throw syntax_error(
        "'" + header + "' is reserved and cannot be used as a section name", m_files[line.file_index], line.line_no);
  }

  return header;
}

std::pair<string, string> config_parser::parse_key(const line_t& line, const string& line_str) {
  size_t pos = line_str.find_first_of('=');

  string key = string_util::trim(line_str.substr(0, pos), isspace);
  string value = string_util::trim(line_str.substr(pos + 1), isspace);

  if (!is_valid_name(key)) {
    throw invalid_name_error("Key", key, m_files[line.file_index], line.line_no);
  }

  value = parse_escaped_value(line, move(value), key);

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

string config_parser::parse_escaped_value(const line_t& line, string&& value, const string& key) {
  string cfg_value = value;
  bool log = false;
  auto backslash_pos = value.find('\\');
  while (backslash_pos != string::npos) {
    if (backslash_pos == value.size() - 1 || value[backslash_pos + 1] != '\\') {
      log = true;
    } else {
      value = value.replace(backslash_pos, 2, "\\");
    }
    backslash_pos = value.find('\\', backslash_pos + 1);
  }
  if (log) {
    m_log.err(
        "%s:%d: Value '%s' of key '%s' contains one or more unescaped backslashes, please prepend them with the "
        "backslash "
        "escape character.",
        m_files[line.file_index], line.line_no, cfg_value, key);
  }
  return move(value);
}
POLYBAR_NS_END

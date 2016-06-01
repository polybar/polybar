#include "utils/config.hpp"
#include "utils/io.hpp"

namespace config
{
  boost::property_tree::ptree pt;
  std::string file_path;
  std::string bar_path;

  void set_bar_path(const std::string &path)
  {
    bar_path = path;
  }

  std::string get_bar_path()
  {
    return bar_path;
  }

  void load(const std::string& path) throw(UnexistingFileError, ParseError)
  {
    if (!io::file::exists(path)) {
      throw UnexistingFileError("Could not find configuration file \""+ path + "\"");
    }

    log_trace(path);

    try {
      boost::property_tree::read_ini(path, pt);
    } catch (std::exception &e) {
      throw ParseError(e.what());
    }

    file_path = path;
  }

  void load(const char *dir, const std::string& path) {
    load(std::string(dir != nullptr ? dir : "") +"/"+ path);
  }

  void reload() throw(ParseError)
  {
    try {
      boost::property_tree::read_ini(file_path, pt);
    } catch (std::exception &e) {
      throw ParseError(e.what());
    }
  }

  boost::property_tree::ptree get_tree() {
    return pt;
  }

  std::string build_path(const std::string& section, const std::string& key) {
    return section +"."+ key;
  }
}

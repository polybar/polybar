#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include "utils/string.hpp"

namespace string
{
  bool compare(const std::string& s1, const std::string& s2) {
    return lower(s1) == lower(s2);
  }

  std::string upper(const std::string& s)
  {
    std::string str(s);
    for (auto &c : str)
      c = std::toupper(c);
    return str;
  }

  std::string lower(const std::string& s)
  {
    std::string str(s);
    for (auto &c : str)
      c = std::tolower(c);
    return str;
  }

  std::string replace(const std::string& haystack, const std::string& needle, const std::string& replacement)
  {
    std::string str(haystack);
    std::string::size_type pos;
    if ((pos = str.find(needle)) != std::string::npos)
      str = str.replace(pos, needle.length(), replacement);
    return str;
  }

  std::string replace_all(const std::string& haystack, const std::string& needle, const std::string& replacement) {
    return boost::replace_all_copy(haystack, needle, replacement);
  }

  std::string squeeze(const std::string& haystack, const char &needle) {
    return replace_all(haystack, {needle, needle}, {needle});
  }

  std::string strip(const std::string& haystack, const char &needle)
  {
    std::string str(haystack);
    std::string::size_type pos;
    while ((pos = str.find(needle)) != std::string::npos)
      str.erase(pos, 1);
    return str;
  }

  std::string strip_trailing_newline(const std::string& haystack)
  {
    std::string str(haystack);
    if (str[str.length()-1] == '\n')
      str.erase(str.length()-1, 1);
    return str;
  }

  std::string trim(const std::string& haystack, const char &needle) {
    return rtrim(ltrim(haystack, needle), needle);
  }

  std::string ltrim(const std::string& haystack, const char &needle)
  {
    std::string str(haystack);
    while (str[0] == needle)
      str.erase(0, 1);
    return str;
  }

  std::string rtrim(const std::string& haystack, const char &needle)
  {
    std::string str(haystack);
    while (str[str.length()-1] == needle)
      str.erase(str.length()-1, 1);
    return str;
  }

  std::string join(const std::vector<std::string> &strs, const std::string& delim)
  {
    std::string str;
    for (auto &s : strs)
      str.append((str.empty() ? "" : delim) + s);
    return str;
  }

  std::vector<std::string> split(const std::string& s, const char &delim)
  {
    std::vector<std::string> vec;
    return split_into(s, delim, vec);
  }

  std::vector<std::string> &split_into(const std::string& s, const char &delim, std::vector<std::string> &container)
  {
    std::string str;
    std::stringstream buffer(s);
    while (std::getline(buffer, str, delim))
      container.emplace_back(str);
    return container;
  }

  std::size_t find_nth(const std::string& haystack, std::size_t pos, const std::string& needle, std::size_t nth)
  {
    std::size_t found_pos = haystack.find(needle, pos);
    if(0 == nth || std::string::npos == found_pos)  return found_pos;
    return find_nth(haystack, found_pos+1, needle, nth-1);
  }
}

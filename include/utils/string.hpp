#ifndef _UTILS_STRING_HPP_
#define _UTILS_STRING_HPP_

#include <string>
#include <vector>

#define STR(s) std::string(s)
#define STRI(s) std::to_string(s)

namespace string
{
  bool compare(const std::string& s1, const std::string& s2);

  std::string upper(const std::string& s);
  std::string lower(const std::string& s);

  std::string replace(const std::string& haystack, const std::string& needle, const std::string& replacement);
  std::string replace_all(const std::string& haystack, const std::string& needle, const std::string& replacement);

  std::string squeeze(const std::string& haystack, const char &needle);

  std::string strip(const std::string& haystack, const char &needle);
  std::string strip_trailing_newline(const std::string& s);

  std::string trim(const std::string& haystack, const char &needle);
  std::string ltrim(const std::string& haystack, const char &needle);
  std::string rtrim(const std::string& haystack, const char &needle);

  std::string join(const std::vector<std::string> &strs, const std::string& delim);

  std::vector<std::string> split(const std::string& s, const char &delim);
  std::vector<std::string> &split_into(const std::string& s, const char &delim, std::vector<std::string> &elems);

  std::size_t find_nth(const std::string& haystack, std::size_t pos, const std::string& needle, std::size_t nth);
}

#endif

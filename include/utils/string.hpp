#pragma once

#include <string>
#include <vector>

namespace string
{
  bool compare(std::string s1, std::string s2);

  // std::string upper(std::string s);
  std::string lower(std::string s);

  std::string replace(std::string haystack, std::string needle, std::string replacement);
  std::string replace_all(std::string haystack, std::string needle, std::string replacement);

  std::string squeeze(std::string haystack, const char &needle);

  // std::string strip(std::string haystack, const char &needle);
  std::string strip_trailing_newline(std::string s);

  std::string trim(std::string haystack, const char &needle);
  std::string ltrim(std::string haystack, const char &needle);
  std::string rtrim(std::string haystack, const char &needle);

  std::string join(const std::vector<std::string> &strs, std::string delim);

  std::vector<std::string> split(std::string s, const char &delim);
  std::vector<std::string> &split_into(std::string s, const char &delim, std::vector<std::string> &elems);

  std::size_t find_nth(std::string haystack, std::size_t pos, std::string needle, std::size_t nth);
}

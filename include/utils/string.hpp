#pragma once

#include <sstream>

#include "common.hpp"

LEMONBUDDY_NS

namespace string_util {
  /**
   * Hash type
   */
  using hash_type = unsigned long;

  bool contains(const string& haystack, const string& needle);
  string upper(const string& s);
  string lower(const string& s);
  bool compare(const string& s1, const string& s2);
  string replace(const string& haystack, string needle, string replacement);
  string replace_all(const string& haystack, string needle, string replacement);
  string squeeze(const string& haystack, char needle);
  string strip(const string& haystack, char needle);
  string strip_trailing_newline(const string& haystack);
  string ltrim(const string& haystack, char needle);
  string rtrim(const string& haystack, char needle);
  string trim(const string& haystack, char needle);
  string join(vector<string> strs, string delim);
  vector<string>& split_into(string s, char delim, vector<string>& container);
  vector<string> split(const string& s, char delim);
  size_t find_nth(string haystack, size_t pos, string needle, size_t nth);
  string from_stream(const std::basic_ostream<char>& os);
  hash_type hash(string src);
}

LEMONBUDDY_NS_END

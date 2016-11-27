#pragma once

#include <sstream>

#include "common.hpp"

POLYBAR_NS

namespace string_util {
  /**
   * Hash type
   */
  using hash_type = unsigned long;

  bool contains(const string& haystack, const string& needle);
  string upper(const string& s);
  string lower(const string& s);
  bool compare(const string& s1, const string& s2);
  string replace(
      const string& haystack, const string& needle, const string& reply, size_t start = 0, size_t end = string::npos);
  string replace_all(
      const string& haystack, const string& needle, const string& reply, size_t start = 0, size_t end = string::npos);
  string replace_all_bounded(const string& haystack, string needle, string replacement, size_t min, size_t max,
      size_t start = 0, size_t end = string::npos);
  string squeeze(const string& haystack, char needle);
  string strip(const string& haystack, char needle);
  string strip_trailing_newline(const string& haystack);
  string ltrim(const string& haystack, char needle);
  string rtrim(const string& haystack, char needle);
  string trim(const string& haystack, char needle);
  string join(const vector<string>& strs, const string& delim);
  vector<string>& split_into(const string& s, char delim, vector<string>& container);
  vector<string> split(const string& s, char delim);
  size_t find_nth(const string& haystack, size_t pos, const string& needle, size_t nth);
  string floatval(float value, int decimals = 2, bool fixed = false, const string& locale = "");
  string filesize(unsigned long long bytes, int decimals = 2, bool fixed = false, const string& locale = "");
  string from_stream(const std::basic_ostream<char>& os);
  hash_type hash(const string& src);
}

POLYBAR_NS_END

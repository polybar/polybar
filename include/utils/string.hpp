#pragma once

#include <sstream>

#include "common.hpp"

POLYBAR_NS

namespace {
  /**
   * Overload that allows sub-string removal at the end of given string
   */
  inline string& operator-(string& a, const string& b) {
    if (a.size() >= b.size() && a.substr(a.size() - b.size()) == b) {
      return a.erase(a.size() - b.size());
    } else {
      return a;
    }
  }

  /**
   * Overload that allows sub-string removal at the end of given string
   */
  inline void operator-=(string& a, const string& b) {
    if (a.size() >= b.size() && a.substr(a.size() - b.size()) == b) {
      a.erase(a.size() - b.size());
    }
  }
}  // namespace

class sstream {
 public:
  sstream() : m_stream() {}

  template <typename T>
  sstream& operator<<(const T& object) {
    m_stream << object;
    return *this;
  }

  sstream& operator<<(const char* cz) {
    m_stream << cz;
    return *this;
  }

  operator string() const {
    return m_stream.str();
  }

  const string to_string() const {
    return m_stream.str();
  }

 private:
  std::stringstream m_stream;
};

namespace string_util {
  /**
   * Hash type
   */
  using hash_type = unsigned long;

  bool contains(const string& haystack, const string& needle);
  string upper(const string& s);
  string lower(const string& s);
  bool compare(const string& s1, const string& s2);

  string replace(const string& haystack, const string& needle, const string& replacement, size_t start = 0,
      size_t end = string::npos);
  string replace_all(const string& haystack, const string& needle, const string& replacement, size_t start = 0,
      size_t end = string::npos);

  string squeeze(const string& haystack, char needle);

  string strip(const string& haystack, char needle);
  string strip_trailing_newline(const string& haystack);

  string ltrim(string value, function<bool(char)> pred);
  string rtrim(string value, function<bool(char)> pred);
  string trim(string value, function<bool(char)> pred);

  string ltrim(string&& value, const char& needle = ' ');
  string rtrim(string&& value, const char& needle = ' ');
  string trim(string&& value, const char& needle = ' ');

  size_t char_len(const string& value);
  string utf8_truncate(string&& value, size_t len);

  string join(const vector<string>& strs, const string& delim);
  vector<string> split(const string& s, char delim);
  std::vector<std::string> tokenize(const string& str, char delimiters);

  size_t find_nth(const string& haystack, size_t pos, const string& needle, size_t nth);

  string floating_point(double value, size_t precision, bool fixed = false, const string& locale = "");
  string filesize_mb(unsigned long long kbytes, size_t precision = 0, const string& locale = "");
  string filesize_gb(unsigned long long kbytes, size_t precision = 0, const string& locale = "");
  string filesize(unsigned long long kbytes, size_t precision = 0, bool fixed = false, const string& locale = "");

  hash_type hash(const string& src);
}  // namespace string_util

POLYBAR_NS_END

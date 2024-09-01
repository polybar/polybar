#pragma once

#include <cstdint>
#include <sstream>

#include "common.hpp"

POLYBAR_NS

class sstream {
 public:
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

  string to_string() const {
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

/**
 * @brief Unicode character containing converted codepoint
 * and details on where its position in the source string
 */
struct unicode_character {
  /**
   * The numerical codepoint. Between U+0000 and U+10FFFF
   */
  uint32_t codepoint{0};
  /**
   * Byte offset of this character in the original string
   */
  int offset{0};
  /**
   * Number of bytes used by this character in the original string
   */
  int length{0};
};
using unicode_charlist = std::vector<unicode_character>;

bool contains(const string& haystack, const string& needle);
bool contains_ignore_case(const string& haystack, const string& needle);
bool ends_with(const string& haystack, const string& suffix);
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
[[nodiscard]] bool utf8_to_ucs4(const string& src, unicode_charlist& result_list);
size_t ucs4_to_utf8(std::array<char, 5>& utf8, unsigned int ucs);

string join(const vector<string>& strs, const string& delim);
vector<string> split(const string& s, char delim);
std::vector<std::string> tokenize(const string& str, char delimiters);

size_t find_nth(const string& haystack, size_t pos, const string& needle, size_t nth);

string floating_point(double value, size_t precision, bool fixed = false, const string& locale = "");
string filesize_mib(unsigned long long kibibytes, size_t precision = 0, const string& locale = "");
string filesize_gib(unsigned long long kibibytes, size_t precision = 0, const string& locale = "");
string filesize_gib_mib(
    unsigned long long kibibytes, size_t precision_mib = 0, size_t precision_gib = 0, const string& locale = "");
string filesize(unsigned long long kbytes, size_t precision = 0, bool fixed = false, const string& locale = "");

hash_type hash(const string& src);
} // namespace string_util

POLYBAR_NS_END

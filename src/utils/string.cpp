#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

#include "utils/string.hpp"

POLYBAR_NS

namespace string_util {
  /**
   * Check if haystack contains needle
   */
  bool contains(const string& haystack, const string& needle) {
    return haystack.find(needle) != string::npos;
  }

  /**
   * Convert string to uppercase
   */
  string upper(const string& s) {
    string str(s);
    for (auto& c : str) {
      c = toupper(c);
    }
    return str;
  }

  /**
   * Convert string to lowercase
   */
  string lower(const string& s) {
    string str(s);
    for (auto& c : str) {
      c = tolower(c);
    }
    return str;
  }

  /**
   * Test lower case equality
   */
  bool compare(const string& s1, const string& s2) {
    return lower(s1) == lower(s2);
  }

  /**
   * Replace first occurence of needle in haystack
   */
  string replace(const string& haystack, const string& needle, const string& replacement, size_t start, size_t end) {
    string str(haystack);
    string::size_type pos;

    if (needle != replacement && (pos = str.find(needle, start)) != string::npos) {
      if (end == string::npos || pos < end) {
        str = str.replace(pos, needle.length(), replacement);
      }
    }

    return str;
  }

  /**
   * Replace all occurences of needle in haystack
   */
  string replace_all(
      const string& haystack, const string& needle, const string& replacement, size_t start, size_t end) {
    string result{haystack};
    string::size_type pos;
    while ((pos = result.find(needle, start)) != string::npos && pos < result.length() &&
           (end == string::npos || pos + needle.length() <= end)) {
      result.replace(pos, needle.length(), replacement);
      start = pos + replacement.length();
    }
    return result;
  }

  /**
   * Replace all consecutive occurrences of needle in haystack
   */
  string squeeze(const string& haystack, char needle) {
    string result = haystack;
    while (result.find({needle, needle}) != string::npos) {
      result = replace_all(result, {needle, needle}, {needle});
    }
    return result;
  }

  /**
   * Remove all occurrences of needle in haystack
   */
  string strip(const string& haystack, char needle) {
    string str(haystack);
    string::size_type pos;
    while ((pos = str.find(needle)) != string::npos) {
      str.erase(pos, 1);
    }
    return str;
  }

  /**
   * Remove trailing newline
   */
  string strip_trailing_newline(const string& haystack) {
    string str(haystack);
    if (str[str.length() - 1] == '\n') {
      str.erase(str.length() - 1, 1);
    }
    return str;
  }

  /**
   * Trims all characters that match pred from the left
   */
  string ltrim(string value, function<bool(char)> pred) {
    value.erase(value.begin(), find_if(value.begin(), value.end(), not1(pred)));
    return value;
  }

  /**
   * Trims all characters that match pred from the right
   */
  string rtrim(string value, function<bool(char)> pred) {
    value.erase(find_if(value.rbegin(), value.rend(), not1(pred)).base(), value.end());
    return value;
  }

  /**
   * Trims all characters that match pred from both sides
   */
  string trim(string value, function<bool(char)> pred) {
    return ltrim(rtrim(move(value), pred), pred);
  }

  /**
   * Remove needle from the start of the string
   */
  string ltrim(string&& value, const char& needle) {
    if (value.empty()) {
      return "";
    }
    while (*value.begin() == needle) {
      value.erase(0, 1);
    }
    return forward<string>(value);
  }

  /**
   * Remove needle from the end of the string
   */
  string rtrim(string&& value, const char& needle) {
    if (value.empty()) {
      return "";
    }
    while (*(value.end() - 1) == needle) {
      value.erase(value.length() - 1, 1);
    }
    return forward<string>(value);
  }

  /**
   * Remove needle from the start and end of the string
   */
  string trim(string&& value, const char& needle) {
    if (value.empty()) {
      return "";
    }
    return rtrim(ltrim(forward<string>(value), needle), needle);
  }

  /**
   * Counts the number of codepoints in a utf8 encoded string.
   */
  size_t char_len(const string& value) {
    // utf-8 bytes of the form 10xxxxxx are continuation bytes, so we
    // simply count the number of bytes not of this form.
    //
    // 0xc0 = 11000000
    // 0x80 = 10000000
    return std::count_if(value.begin(), value.end(), [](char c) { return (c & 0xc0) != 0x80; });
  }

  /**
   * Truncates a utf8 string at len number of codepoints. This isn't 100%
   * matching the user-perceived character count, but it should be close
   * enough and avoids having to pull in something like ICU to count actual
   * grapheme clusters.
   */
  string utf8_truncate(string&& value, size_t len) {
    if (value.empty()) {
      return "";
    }

    // utf-8 bytes of the form 10xxxxxx are continuation bytes, so we
    // simply jump forward to bytes not of that form and truncate starting
    // at that byte if we've counted too many codepoints
    //
    // 0xc0 = 11000000
    // 0x80 = 10000000
    auto it = value.begin();
    auto end = value.end();
    for (size_t i = 0; i < len; ++i) {
      if (it == end)
        break;
      ++it;
      it = std::find_if(it, end, [](char c) { return (c & 0xc0) != 0x80; });
    }
    value.erase(it, end);

    return forward<string>(value);
  }

  /**
   * Join all strings in vector into a single string separated by delim
   */
  string join(const vector<string>& strs, const string& delim) {
    string str;
    for (auto& s : strs) {
      str += (str.empty() ? "" : delim) + s;
    }
    return str;
  }

  /**
   * Explode string by delim, ignore empty tokens
   */
  vector<string> split(const string& s, char delim) {
    std::string::size_type pos = 0;
    std::vector<std::string> result;

    while ((pos = s.find_first_not_of(delim, pos)) != std::string::npos) {
      auto nextpos = s.find_first_of(delim, pos);
      result.emplace_back(s.substr(pos, nextpos - pos));
      pos = nextpos;
    }

    return result;
  }

  /**
   * Explode string by delim, include empty tokens
   */
  std::vector<std::string> tokenize(const string& str, char delimiters) {
    std::vector<std::string> tokens;
    std::string::size_type lastPos = 0;
    auto pos = str.find_first_of(delimiters, lastPos);

    while (pos != std::string::npos && lastPos != std::string::npos) {
      tokens.emplace_back(str.substr(lastPos, pos - lastPos));

      lastPos = pos + 1;
      pos = str.find_first_of(delimiters, lastPos);
    }

    tokens.emplace_back(str.substr(lastPos, pos - lastPos));
    return tokens;
  }

  /**
   * Find the nth occurrence of needle in haystack starting from pos
   */
  size_t find_nth(const string& haystack, size_t pos, const string& needle, size_t nth) {
    size_t found_pos = haystack.find(needle, pos);
    if (1 == nth || string::npos == found_pos) {
      return found_pos;
    }
    return find_nth(haystack, found_pos + 1, needle, nth - 1);
  }

  /**
   * Create a floating point string
   */
  string floating_point(double value, size_t precision, bool fixed, const string& locale) {
    std::stringstream ss;
    ss.imbue(!locale.empty() ? std::locale(locale.c_str()) : std::locale::classic());
    ss << std::fixed << std::setprecision(precision) << value;
    return fixed ? ss.str() : replace(ss.str(), ".00", "");
  }

  /**
   * Create a MB filesize string
   */
  string filesize_mb(unsigned long long kbytes, size_t precision, const string& locale) {
    return floating_point(kbytes / 1024.0, precision, true, locale) + " MB";
  }

  /**
   * Create a GB filesize string
   */
  string filesize_gb(unsigned long long kbytes, size_t precision, const string& locale) {
    return floating_point(kbytes / 1024.0 / 1024.0, precision, true, locale) + " GB";
  }

  /**
   * Create a filesize string by converting given bytes to highest unit possible
   */
  string filesize(unsigned long long bytes, size_t precision, bool fixed, const string& locale) {
    vector<string> suffixes{"TB", "GB", "MB", "KB"};
    string suffix{"B"};
    double value = bytes;
    while (!suffixes.empty() && value >= 1024.0) {
      suffix = suffixes.back();
      suffixes.pop_back();
      value /= 1024.0;
    }
    return floating_point(value, precision, fixed, locale) + " " + suffix;
  }

  /**
   * Compute string hash
   */
  hash_type hash(const string& src) {
    return std::hash<string>()(src);
  }
}  // namespace string_util

POLYBAR_NS_END

#include <cstring>
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
   * Explode string by delim into container
   */
  vector<string>& split_into(const string& s, char delim, vector<string>& container) {
    string str;
    std::stringstream buffer(s);
    while (getline(buffer, str, delim)) {
      container.emplace_back(str);
    }
    return container;
  }

  /**
   * Explode string by delim
   */
  vector<string> split(const string& s, char delim) {
    vector<string> vec;
    return split_into(s, delim, vec);
  }

  /**
   * Find the nth occurence of needle in haystack starting from pos
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
  string filesize(unsigned long long kbytes, size_t precision, bool fixed, const string& locale) {
    vector<string> suffixes{"TB", "GB", "MB"};
    string suffix{"KB"};
    double value = kbytes;
    while (!suffixes.empty() && (value /= 1024.0) >= 1024.0) {
      suffix = suffixes.back();
      suffixes.pop_back();
    }
    return floating_point(value, precision, fixed, locale) + " GB";
  }

  /**
   * Compute string hash
   */
  hash_type hash(const string& src) {
    return std::hash<string>()(src);
  }
}

POLYBAR_NS_END

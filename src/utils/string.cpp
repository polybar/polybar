#include <boost/algorithm/string/replace.hpp>
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
  string replace(const string& haystack, const string& needle, const string& reply, size_t start, size_t end) {
    string str(haystack);
    string::size_type pos;

    if (needle != reply && (pos = str.find(needle, start)) != string::npos) {
      if (end == string::npos || pos < end) {
        str = str.replace(pos, needle.length(), reply);
      }
    }

    return str;
  }

  /**
   * Replace all occurences of needle in haystack
   */
  string replace_all(const string& haystack, const string& needle, const string& reply, size_t start, size_t end) {
    string replaced;

    if (end == string::npos) {
      end = haystack.length();
    }

    for (size_t i = 0; i < haystack.length(); i++) {
      if (i < start) {
        replaced += haystack[i];
      } else if (i + needle.length() > end) {
        replaced += haystack[i];
      } else if (haystack.compare(i, needle.length(), needle) == 0) {
        replaced += reply;
        i += needle.length() - 1;
      } else {
        replaced += haystack[i];
      }
    }

    return replaced;
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
  string ltrim(string&& haystack, const char& needle) {
    string str(haystack);
    while (str[0] == needle) {
      str.erase(0, 1);
    }
    return str;
  }

  /**
   * Remove needle from the end of the string
   */
  string rtrim(string&& haystack, const char& needle) {
    string str(haystack);
    while (str[str.length() - 1] == needle) {
      str.erase(str.length() - 1, 1);
    }
    return str;
  }

  /**
   * Remove needle from the start and end of the string
   */
  string trim(string&& value, const char& needle) {
    return rtrim(ltrim(move(value), needle), needle);
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
    stringstream buffer(s);
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
   * Create a float value string
   */
  string floatval(float value, int decimals, bool fixed, const string& locale) {
    stringstream ss;
    ss.precision(decimals);
    if (!locale.empty()) {
      ss.imbue(std::locale(locale.c_str()));
    }
    if (fixed) {
      ss << std::fixed;
    }
    ss << value;
    return ss.str();
  }

  /**
   * Format a filesize string
   */
  string filesize(unsigned long long bytes, int decimals, bool fixed, const string& locale) {
    vector<string> suffixes{"TB", "GB", "MB"};
    string suffix{"KB"};

    while ((bytes /= 1024) >= 1024) {
      suffix = suffixes.back();
      suffixes.pop_back();
    }

    stringstream ss;
    ss.precision(decimals);
    if (!locale.empty()) {
      ss.imbue(std::locale(locale.c_str()));
    }
    if (fixed) {
      ss << std::fixed;
    }
    ss << bytes << " " << suffix;
    return ss.str();
  }

  /**
   * Get the resulting string from a ostream/
   *
   * Example usage:
   * @code cpp
   *   string_util::from_stream(stringstream() << ...);
   * @endcode
   */
  string from_stream(const std::basic_ostream<char>& os) {
    return static_cast<const stringstream&>(os).str();
  }

  /**
   * Compute string hash
   */
  hash_type hash(const string& src) {
    return std::hash<string>()(src);
  }
}

POLYBAR_NS_END

#include "utils/string.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <utility>

POLYBAR_NS

namespace string_util {

/**
 * Prefixes for the leading byte in a UTF8 codepoint
 */
static constexpr uint8_t UTF8_LEADING1_PREFIX = 0b00000000;
static constexpr uint8_t UTF8_LEADING2_PREFIX = 0b11000000;
static constexpr uint8_t UTF8_LEADING3_PREFIX = 0b11100000;
static constexpr uint8_t UTF8_LEADING4_PREFIX = 0b11110000;

/**
 * Masks to extract the prefix from the leading byte in a UTF8 codepoint
 */
static constexpr uint8_t UTF8_LEADING1_MASK = 0b10000000;
static constexpr uint8_t UTF8_LEADING2_MASK = 0b11100000;
static constexpr uint8_t UTF8_LEADING3_MASK = 0b11110000;
static constexpr uint8_t UTF8_LEADING4_MASK = 0b11111000;

/**
 * Prefix for UTF8 continuation bytes
 */
static constexpr uint8_t UTF8_CONTINUATION_PREFIX = 0b10000000;
/**
 * Mask to extract the UTF8 continuation byte prefix
 */
static constexpr uint8_t UTF8_CONTINUATION_MASK = 0b11000000;

/**
 * Check if haystack contains needle
 */
bool contains(const string& haystack, const string& needle) {
  return haystack.find(needle) != string::npos;
}

bool ends_with(const string& haystack, const string& suffix) {
  if (haystack.length() < suffix.length()) {
    return false;
  }

  return haystack.compare(haystack.length() - suffix.length(), suffix.length(), suffix) == 0;
}

/**
 * Check if haystack contains needle ignoring case
 */
bool contains_ignore_case(const string& haystack, const string& needle) {
  return lower(haystack).find(lower(needle)) != string::npos;
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
 * Replace first occurrence of needle in haystack
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
 * Replace all occurrences of needle in haystack
 */
string replace_all(const string& haystack, const string& needle, const string& replacement, size_t start, size_t end) {
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
  value.erase(value.begin(), find_if(value.begin(), value.end(), std::not_fn(pred)));
  return value;
}

/**
 * Trims all characters that match pred from the right
 */
string rtrim(string value, function<bool(char)> pred) {
  value.erase(find_if(value.rbegin(), value.rend(), std::not_fn(pred)).base(), value.end());
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
  return std::count_if(
      value.begin(), value.end(), [](char c) { return (c & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PREFIX; });
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
  auto it = value.begin();
  auto end = value.end();
  for (size_t i = 0; i < len; ++i) {
    if (it == end)
      break;
    ++it;
    it = std::find_if(it, end, [](char c) { return (c & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_PREFIX; });
  }
  value.erase(it, end);

  return forward<string>(value);
}

/**
 * Given a leading byte of a UTF8 codepoint calculates the number of bytes taken up by the codepoint.
 *
 * @returns {len, result} The codepoint is len bytes and result contains the codepoint bits held in the leading byte.
 */
static pair<int, uint32_t> utf8_get_len(uint8_t leading) {
  if ((leading & UTF8_LEADING1_MASK) == UTF8_LEADING1_PREFIX) {
    return {1, leading & ~UTF8_LEADING1_MASK};
  } else if ((leading & UTF8_LEADING2_MASK) == UTF8_LEADING2_PREFIX) {
    return {2, leading & ~UTF8_LEADING2_MASK};
  } else if ((leading & UTF8_LEADING3_MASK) == UTF8_LEADING3_PREFIX) {
    return {3, leading & ~UTF8_LEADING3_MASK};
  } else if ((leading & UTF8_LEADING4_MASK) == UTF8_LEADING4_PREFIX) {
    return {4, leading & ~UTF8_LEADING4_MASK};
  } else {
    return {-1, 0};
  }
}

/**
 * @brief Create a list of UCS-4 codepoint from a utf-8 encoded string
 *
 * If invalid utf8 characters are encountered they are skipped until the next valid codepoint and the function will
 * eventually return false.
 *
 * The result_list is always populated with all valid utf8 codepoints.
 *
 * @return Whether the string is completely valid utf8
 */
bool utf8_to_ucs4(const string& src, unicode_charlist& result_list) {
  result_list.reserve(src.size());
  bool has_errors = false;
  const auto* begin = reinterpret_cast<const uint8_t*>(src.c_str());

  const auto* current = begin;
  while (*current) {
    // Number of bytes taken up by this codepoint and the bits contained in the leading byte.
    auto [len, result] = utf8_get_len(*current);
    auto offset = current - begin;

    /*
     * Invalid lengths, this byte is not a valid leading byte.
     * Skip it.
     */
    if (len <= 0 || len > 4) {
      has_errors = true;
      current++;
      continue;
    }

    const uint8_t* next = current + 1;
    for (; ((*next & UTF8_CONTINUATION_MASK) == UTF8_CONTINUATION_PREFIX) && (next - current < len); next++) {
      result = result << 6;
      result |= *next & ~UTF8_CONTINUATION_MASK;
    }

    auto actual_len = next - current;
    current = next;

    if (actual_len != len) {
      has_errors = true;
      continue;
    }

    result_list.push_back(unicode_character{result, static_cast<int>(offset), static_cast<int>(actual_len)});
    current = next;
  }

  return !has_errors;
}

/**
 * @brief Convert a UCS-4 codepoint to a utf-8 encoded string
 */
size_t ucs4_to_utf8(std::array<char, 5>& utf8, uint32_t ucs) {
  if (ucs <= 0x7f) {
    utf8[0] = ucs;
    return 1;
  } else if (ucs <= 0x07ff) {
    utf8[0] = ((ucs >> 6) & ~UTF8_LEADING2_MASK) | UTF8_LEADING2_PREFIX;
    utf8[1] = (ucs & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    return 2;
  } else if (ucs <= 0xffff) {
    utf8[0] = ((ucs >> 12) & ~UTF8_LEADING3_MASK) | UTF8_LEADING3_PREFIX;
    utf8[1] = ((ucs >> 6) & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    utf8[2] = (ucs & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    return 3;
  } else if (ucs <= 0x10ffff) {
    utf8[0] = ((ucs >> 18) & ~UTF8_LEADING4_MASK) | UTF8_LEADING4_PREFIX;
    utf8[1] = ((ucs >> 12) & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    utf8[2] = ((ucs >> 6) & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    utf8[3] = (ucs & ~UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_PREFIX;
    return 4;
  } else {
    return 0;
  }
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
 * Create a MiB filesize string
 */
string filesize_mib(unsigned long long kibibytes, size_t precision, const string& locale) {
  return floating_point(kibibytes / 1024.0, precision, true, locale) + " MiB";
}

/**
 * Create a GiB filesize string
 */
string filesize_gib(unsigned long long kibibytes, size_t precision, const string& locale) {
  return floating_point(kibibytes / 1024.0 / 1024.0, precision, true, locale) + " GiB";
}

/**
 * Create a GiB string, if the value in GiB is >= 1.0. Otherwise, create a MiB string.
 */
string filesize_gib_mib(
    unsigned long long kibibytes, size_t precision_mib, size_t precision_gib, const string& locale) {
  if (kibibytes < 1024 * 1024) {
    return filesize_mib(kibibytes, precision_mib, locale);
  } else {
    return filesize_gib(kibibytes, precision_gib, locale);
  }
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
} // namespace string_util

POLYBAR_NS_END

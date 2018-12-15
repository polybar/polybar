#include <cmath>
#include <utility>

#include "drawtypes/label.hpp"
#include "utils/factory.hpp"
#include "utils/string.hpp"

POLYBAR_NS

namespace drawtypes {
  string label::get() const {
    const size_t len = string_util::char_len(m_tokenized);
    if (len >= m_minlen) {
      return m_tokenized;
    }
    const size_t num_fill_chars = m_minlen - len;
    string aligned_label;
    if (m_alignment == alignment::RIGHT) {
      aligned_label = string(num_fill_chars, ' ') + m_tokenized;
    } else if (m_alignment == alignment::LEFT) {
      aligned_label = m_tokenized + string(num_fill_chars, ' ');
    } else {
      const string sided_fill_chars(std::ceil(num_fill_chars / 2.0), ' ');
      aligned_label = sided_fill_chars + m_tokenized + sided_fill_chars;
    }
    return aligned_label;
  }

  label::operator bool() {
    return !m_tokenized.empty();
  }

  label_t label::clone() {
    vector<token> tokens;
    if (!m_tokens.empty()) {
      std::back_insert_iterator<decltype(tokens)> back_it(tokens);
      std::copy(m_tokens.begin(), m_tokens.end(), back_it);
    }
    return factory_util::shared<label>(m_text, m_foreground, m_background, m_underline, m_overline, m_font, m_padding,
        m_margin, m_minlen, m_alignment, m_maxlen, m_ellipsis, move(tokens));
  }

  void label::clear() {
    m_tokenized.clear();
  }

  void label::reset_tokens() {
    m_tokenized = m_text;
  }

  void label::reset_tokens(const string& tokenized) {
    m_tokenized = tokenized;
  }

  bool label::has_token(const string& token) const {
    return m_tokenized.find(token) != string::npos;
  }

  void label::replace_token(const string& token, string replacement) {
    if (!has_token(token)) {
      return;
    }

    for (auto&& tok : m_tokens) {
      string repl{replacement};
      if (token == tok.token) {
        if (tok.max != 0_z && string_util::char_len(repl) > tok.max) {
          repl = string_util::utf8_truncate(std::move(repl), tok.max) + tok.suffix;
        } else if (tok.min != 0_z && repl.length() < tok.min) {
          repl.insert(0_z, tok.min - repl.length(), tok.zpad ? '0' : ' ');
        }

        /*
         * Only replace first occurence, so that the proper token objects can be used
         */
        m_tokenized = string_util::replace(m_tokenized, token, move(repl));
      }
    }
  }

  void label::replace_defined_values(const label_t& label) {
    if (!label->m_foreground.empty()) {
      m_foreground = label->m_foreground;
    }
    if (!label->m_background.empty()) {
      m_background = label->m_background;
    }
    if (!label->m_underline.empty()) {
      m_underline = label->m_underline;
    }
    if (!label->m_overline.empty()) {
      m_overline = label->m_overline;
    }
    if (label->m_font != 0) {
      m_font = label->m_font;
    }
    if (label->m_padding.left != 0U) {
      m_padding.left = label->m_padding.left;
    }
    if (label->m_padding.right != 0U) {
      m_padding.right = label->m_padding.right;
    }
    if (label->m_margin.left != 0U) {
      m_margin.left = label->m_margin.left;
    }
    if (label->m_margin.right != 0U) {
      m_margin.right = label->m_margin.right;
    }
    if (label->m_maxlen != 0_z) {
      m_maxlen = label->m_maxlen;
      m_ellipsis = label->m_ellipsis;
    }
  }

  void label::copy_undefined(const label_t& label) {
    if (m_foreground.empty() && !label->m_foreground.empty()) {
      m_foreground = label->m_foreground;
    }
    if (m_background.empty() && !label->m_background.empty()) {
      m_background = label->m_background;
    }
    if (m_underline.empty() && !label->m_underline.empty()) {
      m_underline = label->m_underline;
    }
    if (m_overline.empty() && !label->m_overline.empty()) {
      m_overline = label->m_overline;
    }
    if (m_font == 0 && label->m_font != 0) {
      m_font = label->m_font;
    }
    if (m_padding.left == 0U && label->m_padding.left != 0U) {
      m_padding.left = label->m_padding.left;
    }
    if (m_padding.right == 0U && label->m_padding.right != 0U) {
      m_padding.right = label->m_padding.right;
    }
    if (m_margin.left == 0U && label->m_margin.left != 0U) {
      m_margin.left = label->m_margin.left;
    }
    if (m_margin.right == 0U && label->m_margin.right != 0U) {
      m_margin.right = label->m_margin.right;
    }
    if (m_maxlen == 0_z && label->m_maxlen != 0_z) {
      m_maxlen = label->m_maxlen;
      m_ellipsis = label->m_ellipsis;
    }
  }

  /**
   * Create a label by loading values from the configuration
   */
  label_t load_label(const config& conf, const string& section, string name, bool required, string def) {
    vector<token> tokens;
    size_t start, end, pos;

    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');

    string text;

    struct side_values padding {
    }, margin{};

    if (required) {
      text = conf.get(section, name);
    } else {
      text = conf.get(section, name, move(def));
    }

    const auto get_left_right = [&](string key) {
      auto value = conf.get(section, key, 0U);
      auto left = conf.get(section, key + "-left", value);
      auto right = conf.get(section, key + "-right", value);
      return side_values{static_cast<unsigned short int>(left), static_cast<unsigned short int>(right)};
    };

    padding = get_left_right(name + "-padding");
    margin = get_left_right(name + "-margin");

    string line{text};

    while ((start = line.find('%')) != string::npos && (end = line.find('%', start + 1)) != string::npos) {
      auto token_str = line.substr(start, end - start + 1);

      // ignore false positives
      //   lemonbar tags %{...}
      //   trailing percentage signs %token%%
      if (token_str.find_first_of("abcdefghijklmnopqrstuvwxyz") != 1) {
        line.erase(0, end);
        continue;
      }

      line.erase(start, end - start + 1);
      tokens.emplace_back(token{token_str, 0_z, 0_z});
      auto& token = tokens.back();

      // find min delimiter
      if ((pos = token_str.find(':')) == string::npos) {
        continue;
      }

      // strip min/max specifiers from the label string token
      token.token = token_str.substr(0, pos) + '%';
      text = string_util::replace(text, token_str, token.token);

      try {
        token.min = std::stoul(&token_str[pos + 1], nullptr, 10);
        // When the number starts with 0 the string is 0-padded
        token.zpad = token_str[pos + 1] == '0';
      } catch (const std::invalid_argument& err) {
        continue;
      }

      // find max delimiter
      if ((pos = token_str.find(':', pos + 1)) == string::npos) {
        continue;
      }

      try {
        token.max = std::stoul(&token_str[pos + 1], nullptr, 10);
      } catch (const std::invalid_argument& err) {
        continue;
      }

      // ignore max lengths less than min
      if (token.max < token.min) {
        token.max = 0_z;
      }

      // find suffix delimiter
      if ((pos = token_str.find(':', pos + 1)) != string::npos) {
        token.suffix = token_str.substr(pos + 1, token_str.size() - pos - 2);
      }
    }
    size_t minlen = conf.get(section, name + "-minlen", 0_z);
    string alignment_conf_value = conf.get(section, name + "-alignment", "right"s);
    alignment label_alignment;
    if (alignment_conf_value == "right") {
      label_alignment = alignment::RIGHT;
    } else if (alignment_conf_value == "left") {
      label_alignment = alignment::LEFT;
    } else if (alignment_conf_value == "center") {
      label_alignment = alignment::CENTER;
    } else {
      throw application_error(sstream() << "Label " << section << "." << name << " has invalid alignment "
                                        << alignment_conf_value << ", expecting one of: right, left, center.");
    }

    size_t maxlen = conf.get(section, name + "-maxlen", 0_z);
    if (maxlen > 0 && maxlen < minlen) {
      throw application_error(sstream() << "Label " << section << "." << name << " has maxlen " << maxlen
                                        << " which is smaller than minlen " << minlen);
    }
    bool ellipsis = conf.get(section, name + "-ellipsis", true);

    if(ellipsis && maxlen > 0 && maxlen < 3) {
      throw application_error(sstream()
          << "Label " << section << "." << name
          << " has maxlen " << maxlen
          << ", which is smaller than length of ellipsis (3)");
    }

    // clang-format off
    return factory_util::shared<label>(text,
        conf.get(section, name + "-foreground", ""s),
        conf.get(section, name + "-background", ""s),
        conf.get(section, name + "-underline", ""s),
        conf.get(section, name + "-overline", ""s),
        conf.get(section, name + "-font", 0),
        padding,
        margin,
        minlen,
        label_alignment,
        maxlen,
        ellipsis,
        move(tokens));
    // clang-format on
  }

  /**
   * Create a label by loading optional values from the configuration
   */
  label_t load_optional_label(const config& conf, string section, string name, string def) {
    return load_label(conf, move(section), move(name), false, move(def));
  }

}

POLYBAR_NS_END

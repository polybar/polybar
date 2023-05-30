#include "drawtypes/label.hpp"

#include <cmath>
#include <utility>

#include "utils/string.hpp"

POLYBAR_NS

namespace drawtypes {
  /**
   * Gets the text from the label as it should be rendered
   *
   * Here tokens are replaced with values and minlen and maxlen properties are applied
   */
  string label::get() const {
    const size_t len = string_util::char_len(m_tokenized);
    if (len >= m_minlen) {
      string text = m_tokenized;
      if (m_maxlen > 0 && len > m_maxlen) {
        if (m_ellipsis) {
          text = string_util::utf8_truncate(std::move(text), m_maxlen - 3) + "...";
        } else {
          text = string_util::utf8_truncate(std::move(text), m_maxlen);
        }
      }
      return text;
    }

    const size_t num_fill_chars = m_minlen - len;
    size_t right_fill_len = 0;
    size_t left_fill_len = 0;
    if (m_alignment == alignment::RIGHT) {
      left_fill_len = num_fill_chars;
    } else if (m_alignment == alignment::LEFT) {
      right_fill_len = num_fill_chars;
    } else {
      right_fill_len = std::ceil(num_fill_chars / 2.0);
      left_fill_len = right_fill_len;
      // The text is positioned one character to the left if we can't perfectly center it
      if (len + left_fill_len + right_fill_len > m_minlen) {
        --left_fill_len;
      }
    }
    return string(left_fill_len, ' ') + m_tokenized + string(right_fill_len, ' ');
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
    return std::make_shared<label>(m_text, m_foreground, m_background, m_underline, m_overline, m_font, m_padding,
        m_margin, m_minlen, m_maxlen, m_alignment, m_ellipsis, move(tokens));
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
          if (tok.rpadding) {
            repl.append(tok.min - repl.length(), ' ');
          } else {
            repl.insert(0_z, tok.min - repl.length(), tok.zpad ? '0' : ' ');
          }
        }

        /*
         * Only replace first occurence, so that the proper token objects can be used
         */
        m_tokenized = string_util::replace(m_tokenized, token, move(repl));
      }
    }
  }

  void label::replace_defined_values(const label_t& label) {
    if (label->m_foreground.has_color()) {
      m_foreground = label->m_foreground;
    }
    if (label->m_background.has_color()) {
      m_background = label->m_background;
    }
    if (label->m_underline.has_color()) {
      m_underline = label->m_underline;
    }
    if (label->m_overline.has_color()) {
      m_overline = label->m_overline;
    }
    if (label->m_font != 0) {
      m_font = label->m_font;
    }
    if (label->m_padding.left) {
      m_padding.left = label->m_padding.left;
    }
    if (label->m_padding.right) {
      m_padding.right = label->m_padding.right;
    }
    if (label->m_margin.left) {
      m_margin.left = label->m_margin.left;
    }
    if (label->m_margin.right) {
      m_margin.right = label->m_margin.right;
    }
    if (label->m_maxlen != 0_z) {
      m_maxlen = label->m_maxlen;
      m_ellipsis = label->m_ellipsis;
    }
  }

  void label::copy_undefined(const label_t& label) {
    if (!m_foreground.has_color() && label->m_foreground.has_color()) {
      m_foreground = label->m_foreground;
    }
    if (!m_background.has_color() && label->m_background.has_color()) {
      m_background = label->m_background;
    }
    if (!m_underline.has_color() && label->m_underline.has_color()) {
      m_underline = label->m_underline;
    }
    if (!m_overline.has_color() && label->m_overline.has_color()) {
      m_overline = label->m_overline;
    }
    if (m_font == 0 && label->m_font != 0) {
      m_font = label->m_font;
    }
    if (!m_padding.left && label->m_padding.left) {
      m_padding.left = label->m_padding.left;
    }
    if (!m_padding.right && label->m_padding.right) {
      m_padding.right = label->m_padding.right;
    }
    if (!m_margin.left && label->m_margin.left) {
      m_margin.left = label->m_margin.left;
    }
    if (!m_margin.right && label->m_margin.right) {
      m_margin.right = label->m_margin.right;
    }
    if (m_maxlen == 0_z && label->m_maxlen != 0_z) {
      m_maxlen = label->m_maxlen;
      m_ellipsis = label->m_ellipsis;
    }
  }

  /**
   * Create a label by loading optional values from the configuration
   */
  label_t load_optional_label(const config& conf, string section, string name, string def) {
    return load_label(conf, section, move(name), false, move(def));
  }

  /**
   * Create a separator from the configuration
   */
  label_t load_separator(const config& conf, string name) {
    return load_label<BarTrait>(conf, "", move(name), false);
  }
} // namespace drawtypes

POLYBAR_NS_END

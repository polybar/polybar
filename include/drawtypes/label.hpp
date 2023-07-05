#pragma once

#include <cassert>

#include "common.hpp"
#include "components/config.hpp"
#include "components/types.hpp"
#include "utils/mixins.hpp"

POLYBAR_NS

namespace drawtypes {
  struct token {
    string token;
    size_t min{0_z};
    size_t max{0_z};
    string suffix{""s};
    bool zpad{false};
    bool rpadding{false};
  };

  class label : public non_copyable_mixin {
   public:
    rgba m_foreground{};
    rgba m_background{};
    rgba m_underline{};
    rgba m_overline{};
    int m_font{0};
    side_values m_padding{ZERO_SPACE, ZERO_SPACE};
    side_values m_margin{ZERO_SPACE, ZERO_SPACE};

    size_t m_minlen{0};
    /*
     * If m_ellipsis is true, m_maxlen MUST be larger or equal to the length of
     * the ellipsis (3), everything else is a programming error
     *
     * load_label should take care of this, but be aware, if you are creating
     * labels in a different way.
     */
    size_t m_maxlen{0_z};
    alignment m_alignment{alignment::LEFT};
    bool m_ellipsis{true};

    explicit label(string text, int font) : m_font(font), m_text(move(text)), m_tokenized(m_text) {}
    explicit label(string text, rgba foreground = rgba{}, rgba background = rgba{}, rgba underline = rgba{},
        rgba overline = rgba{}, int font = 0, side_values padding = {ZERO_SPACE, ZERO_SPACE},
        side_values margin = {ZERO_SPACE, ZERO_SPACE}, int minlen = 0, size_t maxlen = 0_z,
        alignment label_alignment = alignment::LEFT, bool ellipsis = true, vector<token>&& tokens = {})
        : m_foreground(move(foreground))
        , m_background(move(background))
        , m_underline(move(underline))
        , m_overline(move(overline))
        , m_font(font)
        , m_padding(padding)
        , m_margin(margin)
        , m_minlen(minlen)
        , m_maxlen(maxlen)
        , m_alignment(label_alignment)
        , m_ellipsis(ellipsis)
        , m_text(move(text))
        , m_tokenized(m_text)
        , m_tokens(forward<vector<token>>(tokens)) {
      assert(!m_ellipsis || (m_maxlen == 0 || m_maxlen >= 3));
    }

    string get() const;
    explicit operator bool();
    label_t clone();
    void clear();
    void reset_tokens();
    void reset_tokens(const string& tokenized);
    bool has_token(const string& token) const;
    void replace_token(const string& token, string replacement);
    void replace_defined_values(const label_t& label);
    void copy_undefined(const label_t& label);

   private:
    string m_text{};
    string m_tokenized{};
    const vector<token> m_tokens{};
  };

  class ModuleTrait {
   public:
    template<typename T = string>
    static T get(const config& conf, string section, string name) {
      return conf.get(section, name);
    }
    template<typename T = string>
    static T get(const config& conf, string section, string name, const T& default_value) {
      return conf.get(section, name, default_value);
    }
  };

  class BarTrait {
   public:
    template<typename T = string>
    static T get(const config& conf, string, string name) {
      return conf.bar_get(name);
    }
    template<typename T = string>
    static T get(const config& conf, string, string name, const T& default_value) {
      return conf.bar_get(name, default_value);
    }
  };

  label_t load_optional_label(const config& conf, string section, string name, string def = ""s);
  label_t load_separator(const config& conf, string name);
  /**
   * Create a label by loading values from the configuration
   */
  template<typename T = ModuleTrait>
  label_t load_label(const config& conf, const string& section, string name, bool required = true, string def = ""s) {
    vector<token> tokens;
    size_t start, end, pos;

    name = string_util::ltrim(string_util::rtrim(move(name), '>'), '<');

    string text;

    struct side_values padding {
    }, margin{};

    if (required) {
      text = T::get(conf, section, name);
    } else {
      text = T::get(conf, section, name, def);
    }

    const auto get_left_right = [&](string&& key) {
      const auto parse_or_throw = [&](const string& key, spacing_val default_value) {
        try {
          return T::get(conf, section, key, default_value);
        } catch (const std::exception& err) {
          throw application_error(
              sstream() << "Failed to set " << section << "." << key << " (reason: " << err.what() << ")");
        }
      };

      auto value = parse_or_throw(key, ZERO_SPACE);
      auto left = parse_or_throw(key + "-left", value);
      auto right = parse_or_throw(key + "-right", value);
      return side_values{left, right};
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
        if (token_str[pos + 1] == '-') {
          token.rpadding = true;
          pos++;
        }
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
    size_t minlen = T::get(conf, section, name + "-minlen", 0_z);
    string alignment_conf_value = T::get(conf, section, name + "-alignment", "left"s);
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

    size_t maxlen = T::get(conf, section, name + "-maxlen", 0_z);
    if (maxlen > 0 && maxlen < minlen) {
      throw application_error(sstream() << "Label " << section << "." << name << " has maxlen " << maxlen
                                        << " which is smaller than minlen " << minlen);
    }
    bool ellipsis = T::get(conf, section, name + "-ellipsis", true);

    // clang-format off
    if (ellipsis && maxlen > 0 && maxlen < 3) {
      throw application_error(sstream() << "Label " << section << "." << name << " has maxlen " << maxlen
                                        << ", which is smaller than length of ellipsis (3)");
    }
    // clang-format on

    // clang-format off
    return std::make_shared<label>(text,
        T::get(conf, section, name + "-foreground", rgba{}),
        T::get(conf, section, name + "-background", rgba{}),
        T::get(conf, section, name + "-underline", rgba{}),
        T::get(conf, section, name + "-overline", rgba{}),
        T::get(conf, section, name + "-font", 0),
        padding,
        margin,
        minlen,
        maxlen,
        label_alignment,
        ellipsis,
        move(tokens));
    // clang-format on
  }

  label_t load_optional_label(const config::value& conf, string def = ""s);

  /**
   * Create a label by loading values from the configuration value
   */
  label_t load_label(const config::value& conf, bool required = true, string def = ""s);
} // namespace drawtypes

POLYBAR_NS_END

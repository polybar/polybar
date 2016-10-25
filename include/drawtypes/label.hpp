#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  class label;
  using icon = label;
  using label_t = shared_ptr<label>;
  using icon_t = label_t;

  class label : public non_copyable_mixin<label> {
   public:
    string m_foreground;
    string m_background;
    string m_underline;
    string m_overline;
    int m_font = 0;
    int m_padding = 0;
    int m_margin = 0;
    size_t m_maxlen = 0;
    bool m_ellipsis = true;

    explicit label(string text, int font) : m_font(font), m_text(text), m_tokenized(m_text) {}
    explicit label(string text, string foreground = "", string background = "",
        string underline = "", string overline = "", int font = 0, int padding = 0, int margin = 0,
        size_t maxlen = 0, bool ellipsis = true)
        : m_foreground(foreground)
        , m_background(background)
        , m_underline(underline)
        , m_overline(overline)
        , m_font(font)
        , m_padding(padding)
        , m_margin(margin)
        , m_maxlen(maxlen)
        , m_ellipsis(ellipsis)
        , m_text(text)
        , m_tokenized(m_text) {}

    string get() const {
      return m_tokenized;
    }

    operator bool() {
      return !m_text.empty();
    }

    label_t clone() {
      return label_t{new label(m_text, m_foreground, m_background, m_underline, m_overline, m_font,
          m_padding, m_margin, m_maxlen, m_ellipsis)};
    }

    void reset_tokens() {
      m_tokenized = m_text;
    }

    void replace_token(string token, string replacement) {
      m_tokenized = string_util::replace_all(m_tokenized, token, replacement);
    }

    void replace_defined_values(const label_t& label) {
      if (!label->m_foreground.empty())
        m_foreground = label->m_foreground;
      if (!label->m_background.empty())
        m_background = label->m_background;
      if (!label->m_underline.empty())
        m_underline = label->m_underline;
      if (!label->m_overline.empty())
        m_overline = label->m_overline;
    }

    void copy_undefined(const label_t& label) {
      if (m_foreground.empty() && !label->m_foreground.empty())
        m_foreground = label->m_foreground;
      if (m_background.empty() && !label->m_background.empty())
        m_background = label->m_background;
      if (m_underline.empty() && !label->m_underline.empty())
        m_underline = label->m_underline;
      if (m_overline.empty() && !label->m_overline.empty())
        m_overline = label->m_overline;
      if (m_font == 0 && label->m_font != 0)
        m_font = label->m_font;
      if (m_padding == 0 && label->m_padding != 0)
        m_padding = label->m_padding;
      if (m_margin == 0 && label->m_margin != 0)
        m_margin = label->m_margin;
      if (m_maxlen == 0 && label->m_maxlen != 0) {
        m_maxlen = label->m_maxlen;
        m_ellipsis = label->m_ellipsis;
      }
    }

   private:
    string m_text, m_tokenized;
  };

  /**
   * Create a label by loading values from the configuration
   */
  inline label_t load_label(
      const config& conf, string section, string name, bool required = true, string def = "") {
    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    string text;

    if (required)
      text = conf.get<string>(section, name);
    else
      text = conf.get<string>(section, name, def);

    // clang-format off
    return label_t{new label_t::element_type(text,
        conf.get<string>(section, name + "-foreground", ""),
        conf.get<string>(section, name + "-background", ""),
        conf.get<string>(section, name + "-underline", ""),
        conf.get<string>(section, name + "-overline", ""),
        conf.get<int>(section, name + "-font", 0),
        conf.get<int>(section, name + "-padding", 0),
        conf.get<int>(section, name + "-margin", 0),
        conf.get<size_t>(section, name + "-maxlen", 0),
        conf.get<bool>(section, name + "-ellipsis", true))};
    // clang-format on
  }

  /**
   * Create a label by loading optional values from the configuration
   */
  inline label_t load_optional_label(
      const config& conf, string section, string name, string def = "") {
    return load_label(conf, section, name, false, def);
  }

  /**
   * Create an icon by loading values from the configuration
   */
  inline icon_t load_icon(
      const config& conf, string section, string name, bool required = true, string def = "") {
    return load_label(conf, section, name, required, def);
  }

  /**
   * Create an icon by loading optional values from the configuration
   */
  inline icon_t load_optional_icon(
      const config& conf, string section, string name, string def = "") {
    return load_icon(conf, section, name, false, def);
  }
}

LEMONBUDDY_NS_END

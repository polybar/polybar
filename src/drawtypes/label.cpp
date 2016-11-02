#include "drawtypes/label.hpp"

LEMONBUDDY_NS

namespace drawtypes {
  string label::get() const {
    return m_tokenized;
  }

  label::operator bool() {
    return !m_text.empty();
  }

  label_t label::clone() {
    return label_t{new label(m_text, m_foreground, m_background, m_underline, m_overline, m_font,
        m_padding, m_margin, m_maxlen, m_ellipsis)};
  }

  void label::reset_tokens() {
    m_tokenized = m_text;
  }

  void label::replace_token(string token, string replacement) {
    m_tokenized = string_util::replace_all(m_tokenized, token, replacement);
  }

  void label::replace_defined_values(const label_t& label) {
    if (!label->m_foreground.empty())
      m_foreground = label->m_foreground;
    if (!label->m_background.empty())
      m_background = label->m_background;
    if (!label->m_underline.empty())
      m_underline = label->m_underline;
    if (!label->m_overline.empty())
      m_overline = label->m_overline;
  }

  void label::copy_undefined(const label_t& label) {
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

  /**
   * Create a label by loading values from the configuration
   */
  label_t load_label(const config& conf, string section, string name, bool required, string def) {
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
  label_t load_optional_label(const config& conf, string section, string name, string def) {
    return load_label(conf, section, name, false, def);
  }

  /**
   * Create an icon by loading values from the configuration
   */
  icon_t load_icon(const config& conf, string section, string name, bool required, string def) {
    return load_label(conf, section, name, required, def);
  }

  /**
   * Create an icon by loading optional values from the configuration
   */
  icon_t load_optional_icon(const config& conf, string section, string name, string def) {
    return load_icon(conf, section, name, false, def);
  }
}

LEMONBUDDY_NS_END

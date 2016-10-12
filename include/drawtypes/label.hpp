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
    string m_text{""};
    string m_foreground{""};
    string m_background{""};
    string m_underline{""};
    string m_overline{""};
    int m_font{0};
    int m_padding{0};
    int m_margin{0};
    size_t m_maxlen{0};
    bool m_ellipsis{true};

    explicit label(string text, int font) : m_text(text), m_font(font) {}
    explicit label(string text, string foreground = "", string background = "",
        string underline = "", string overline = "", int font = 0, int padding = 0, int margin = 0,
        size_t maxlen = 0, bool ellipsis = true)
        : m_text(text)
        , m_foreground(foreground)
        , m_background(background)
        , m_underline(underline)
        , m_overline(overline)
        , m_font(font)
        , m_padding(padding)
        , m_margin(margin)
        , m_maxlen(maxlen)
        , m_ellipsis(ellipsis) {}

    operator bool() {
      return !m_text.empty();
    }

    label_t clone() {
      return label_t{new label(m_text, m_foreground, m_background, m_underline, m_overline, m_font,
          m_padding, m_margin, m_maxlen, m_ellipsis)};
    }

    void replace_token(string token, string replacement) {
      m_text = string_util::replace_all(m_text, token, replacement);
    }

    void replace_defined_values(label_t label) {
      if (!label->m_foreground.empty())
        m_foreground = label->m_foreground;
      if (!label->m_background.empty())
        m_background = label->m_background;
      if (!label->m_underline.empty())
        m_underline = label->m_underline;
      if (!label->m_overline.empty())
        m_overline = label->m_overline;
    }
  };

  inline label_t get_config_label(const config& conf, string section, string name = "label",
      bool required = true, string def = "") {
    string text;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    if (required)
      text = conf.get<string>(section, name);
    else
      text = conf.get<string>(section, name, def);

    return label_t{new label(text, conf.get<string>(section, name + "-foreground", ""),
        conf.get<string>(section, name + "-background", ""),
        conf.get<string>(section, name + "-underline", ""),
        conf.get<string>(section, name + "-overline", ""),
        conf.get<int>(section, name + "-font", 0), conf.get<int>(section, name + "-padding", 0),
        conf.get<int>(section, name + "-margin", 0), conf.get<size_t>(section, name + "-maxlen", 0),
        conf.get<bool>(section, name + "-ellipsis", true))};
  }

  inline label_t get_optional_config_label(
      const config& conf, string section, string name = "label", string def = "") {
    return get_config_label(conf, section, name, false, def);
  }

  inline icon_t get_config_icon(const config& conf, string section, string name = "icon",
      bool required = true, string def = "") {
    return get_config_label(conf, section, name, required, def);
  }

  inline icon_t get_optional_config_icon(
      const config& conf, string section, string name = "icon", string def = "") {
    return get_config_icon(conf, section, name, false, def);
  }
}

LEMONBUDDY_NS_END

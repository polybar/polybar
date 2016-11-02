#pragma once

#include "common.hpp"
#include "components/config.hpp"
#include "utils/mixins.hpp"

LEMONBUDDY_NS

/**
 * TODO: Remove icon_t
 */

namespace drawtypes {
  class label;
  using label_t = shared_ptr<label>;

  /**
   * @decprecated: use label
   */
  using icon = label;
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

    string get() const;
    operator bool();
    label_t clone();
    void reset_tokens();
    void replace_token(string token, string replacement);
    void replace_defined_values(const label_t& label);
    void copy_undefined(const label_t& label);

   private:
    string m_text, m_tokenized;
  };

  label_t load_label(
      const config& conf, string section, string name, bool required = true, string def = "");

  label_t load_optional_label(const config& conf, string section, string name, string def = "");

  icon_t load_icon(
      const config& conf, string section, string name, bool required = true, string def = "");

  icon_t load_optional_icon(const config& conf, string section, string name, string def = "");
}

LEMONBUDDY_NS_END

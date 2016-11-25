#include "drawtypes/label.hpp"

POLYBAR_NS

namespace drawtypes {
  string label::get() const {
    return m_tokenized;
  }

  label::operator bool() {
    return !m_text.empty();
  }

  label_t label::clone() {
    return label_t{new label(m_text, m_foreground, m_background, m_underline, m_overline, m_font, m_padding, m_margin,
        m_maxlen, m_ellipsis, m_token_bounds)};
  }

  void label::reset_tokens() {
    m_tokenized = m_text;
  }

  bool label::has_token(string token) {
    return m_text.find(token) != string::npos;
  }

  void label::replace_token(string token, string replacement) {
    if (!has_token(token))
      return;

    for (auto&& bound : m_token_bounds) {
      if (token != bound.token)
        continue;
      m_tokenized = string_util::replace_all_bounded(m_tokenized, token, replacement, bound.min, bound.max);
    }
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
    vector<struct bounds> bound;
    size_t start, end, pos;

    name = string_util::ltrim(string_util::rtrim(name, '>'), '<');

    string text;

    if (required)
      text = conf.get<string>(section, name);
    else
      text = conf.get<string>(section, name, def);

    string line{text};

    while ((start = line.find('%')) != string::npos && (end = line.find('%', start + 1)) != string::npos) {
      auto token = line.substr(start, end - start + 1);

      // ignore false positives (lemonbar-style declarations)
      if (token[1] == '{') {
        line.erase(0, start + 1);
        continue;
      }

      line.erase(start, end - start + 1);
      bound.emplace_back(bounds{token, 0, 0});

      // find min delimiter
      if ((pos = token.find(':')) == string::npos)
        continue;

      // strip min/max specifiers from the label string token
      bound.back().token = token.substr(0, pos) + '%';
      text = string_util::replace(text, token, bound.back().token);

      try {
        bound.back().min = std::stoul(&token[pos + 1], nullptr, 10);
      } catch (const std::invalid_argument& err) {
        continue;
      }

      // find max delimiter
      if ((pos = token.find(':', pos + 1)) == string::npos)
        continue;

      try {
        bound.back().max = std::stoul(&token[pos + 1], nullptr, 10);
      } catch (const std::invalid_argument& err) {
        continue;
      }

      // ignore max lengths less than min
      if (bound.back().max < bound.back().min)
        bound.back().max = 0;
    }

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
        conf.get<bool>(section, name + "-ellipsis", true),
        bound)};
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

POLYBAR_NS_END

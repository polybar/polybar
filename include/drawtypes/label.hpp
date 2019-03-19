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
  };

  class label;
  using label_t = shared_ptr<label>;

  class label : public non_copyable_mixin<label> {
   public:
    string m_foreground{};
    string m_background{};
    string m_underline{};
    string m_overline{};
    int m_font{0};
    side_values m_padding{{space_type::SPACE, 0U}, {space_type::SPACE, 0U}};
    side_values m_margin{{space_type::SPACE, 0U}, {space_type::SPACE, 0U}};

    /*
     * If m_ellipsis is true, m_maxlen MUST be larger or equal to the length of
     * the ellipsis (3), everything else is a programming error
     *
     * load_label should take care of this, but be aware, if you are creating
     * labels in a different way.
     */
    size_t m_maxlen{0_z};
    bool m_ellipsis{true};

    explicit label(string text, int font) : m_font(font), m_text(move(text)), m_tokenized(m_text) {}
    explicit label(string text, string foreground = ""s, string background = ""s, string underline = ""s,
        string overline = ""s, int font = 0, side_values padding = {{space_type::SPACE, 0U}, {space_type::SPACE, 0U}},
        side_values margin = {{space_type::SPACE, 0U}, {space_type::SPACE, 0U}}, size_t maxlen = 0_z,
        bool ellipsis = true, vector<token>&& tokens = {})
        : m_foreground(move(foreground))
        , m_background(move(background))
        , m_underline(move(underline))
        , m_overline(move(overline))
        , m_font(font)
        , m_padding(padding)
        , m_margin(margin)
        , m_maxlen(maxlen)
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

  label_t load_label(const config& conf, const string& section, string name, bool required = true, string def = ""s);
  label_t load_optional_label(const config& conf, string section, string name, string def = ""s);
}  // namespace drawtypes

POLYBAR_NS_END

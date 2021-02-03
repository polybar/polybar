#pragma once

#include "common.hpp"
#include "components/types.hpp"
#include "tags/types.hpp"
#include "utils/color.hpp"

POLYBAR_NS

namespace tags {
  /**
   * Stores all the formatting data which comes from formatting tags needed to
   * render a formatting string.
   */
  class context {
   public:
    context() = delete;
    context(const bar_settings& settings);

    /**
     * Resets to the initial state.
     *
     * The initial state depends on the colors set on the bar itself.
     */
    void reset();

    void apply_bg(color_value c);
    void apply_fg(color_value c);
    void apply_ol(color_value c);
    void apply_ul(color_value c);
    void apply_font(int font);
    void apply_reverse();
    void apply_alignment(alignment align);
    void apply_attr(attr_activation act, attribute attr);
    void apply_reset();

    rgba get_bg() const;
    rgba get_fg() const;
    rgba get_ol() const;
    rgba get_ul() const;
    int get_font() const;
    bool has_overline() const;
    bool has_underline() const;
    alignment get_alignment() const;

   protected:
    /**
     * Background color
     */
    rgba m_bg;
    /**
     * Foreground color
     */
    rgba m_fg;
    /**
     * Overline color
     */
    rgba m_ol;
    /**
     * Underline color
     */
    rgba m_ul;
    /**
     * Font index (1-based)
     */
    int m_font;
    /**
     * Is overline enabled?
     */
    bool m_attr_overline;
    /**
     * Is underline enabled?
     */
    bool m_attr_underline;
    /**
     * Alignment block
     */
    alignment m_align;

   private:
    const bar_settings& m_settings;
  };
}  // namespace tags

POLYBAR_NS_END

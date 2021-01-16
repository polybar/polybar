#pragma once

#include <map>

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

  /**
   * An identifier for an action block.
   *
   * A value of NO_ACTION denotes an undefined identifier and is guaranteed to
   * be smaller (<) than any valid identifier.
   *
   * If two action blocks overlap, the action with the higher identifier will
   * be above.
   *
   * Except for NO_ACTION, negative values are not allowed
   */
  using action_t = int;

  static constexpr action_t NO_ACTION = -1;

  struct action_block {
    action_block(const string&& cmd, mousebtn button, alignment align, bool is_open)
        : cmd(std::move(cmd)), button(button), align(align), is_open(is_open){};

    string cmd;
    double start_x{0};
    double end_x{0};
    mousebtn button;
    alignment align;
    bool is_open;

    unsigned int width() const {
      return static_cast<unsigned int>(end_x - start_x + 0.5);
    }

    bool test(int point) const {
      return static_cast<int>(start_x) <= point && static_cast<int>(end_x) > point;
    }
  };

  class action_context {
   public:
    void reset();

    action_t action_open(mousebtn btn, const string&& cmd, alignment align);
    std::pair<action_t, mousebtn> action_close(mousebtn btn, alignment align);

    void set_start(action_t id, double x);
    void set_end(action_t id, double x);

    std::map<mousebtn, tags::action_t> get_actions(int x) const;
    action_t has_action(mousebtn btn, int x) const;

    string get_action(action_t id) const;
    bool has_double_click() const;

    size_t num_actions() const;

    // TODO provide better interface for adjusting the start positions of actions
    std::vector<action_block>& get_blocks();

   protected:
    /**
     * Stores all currently known action blocks.
     *
     * The action_t type is meant as an index into this vector.
     */
    std::vector<action_block> m_action_blocks;
  };

}  // namespace tags

POLYBAR_NS_END

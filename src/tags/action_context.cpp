#include "tags/action_context.hpp"

#include <cassert>

POLYBAR_NS

namespace tags {

  void action_context::reset() {
    m_action_blocks.clear();
  }

  action_t action_context::action_open(mousebtn btn, const string&& cmd, alignment align, double x) {
    action_t id = m_action_blocks.size();
    m_action_blocks.emplace_back(std::move(cmd), btn, align, true);
    set_start(id, x);
    return id;
  }

  std::pair<action_t, mousebtn> action_context::action_close(mousebtn btn, alignment align, double x) {
    for (auto it = m_action_blocks.rbegin(); it != m_action_blocks.rend(); it++) {
      if (it->is_open && it->align == align && (btn == mousebtn::NONE || it->button == btn)) {
        it->is_open = false;

        // Converts a reverse iterator into an index
        action_t id = std::distance(m_action_blocks.begin(), it.base()) - 1;
        set_end(id, x);
        return {id, it->button};
      }
    }

    return {NO_ACTION, mousebtn::NONE};
  }

  void action_context::set_start(action_t id, double x) {
    m_action_blocks[id].start_x = x;
  }

  void action_context::set_end(action_t id, double x) {
    /*
     * Only ever increase the end position.
     * A larger end position may have been set before.
     */
    m_action_blocks[id].end_x = std::max(m_action_blocks[id].end_x, x);
  }

  void action_context::compensate_for_negative_move(alignment a, double old_x, double new_x) {
    assert(new_x < old_x);
    for (auto& block : m_action_blocks) {
      if (block.is_open && block.align == a) {
        // Move back the start position if a smaller position is observed
        if (block.start_x > new_x) {
          block.start_x = new_x;
        }

        // Move forward the end position if a larger position is observed
        if (old_x > block.end_x) {
          block.end_x = old_x;
        }
      }
    }
  }

  void action_context::set_alignment_start(const alignment a, const double x) {
    m_align_start[a] = x;
  }

  std::map<mousebtn, tags::action_t> action_context::get_actions(int x) const {
    std::map<mousebtn, tags::action_t> buttons;

    for (int i = static_cast<int>(mousebtn::NONE); i < static_cast<int>(mousebtn::BTN_COUNT); i++) {
      buttons[static_cast<mousebtn>(i)] = tags::NO_ACTION;
    }

    for (action_t id = 0; (unsigned)id < m_action_blocks.size(); id++) {
      auto action = m_action_blocks[id];
      mousebtn btn = action.button;

      // Higher IDs are higher in the action stack.
      if (id > buttons[btn] && action.test(m_align_start.at(action.align), x)) {
        buttons[action.button] = id;
      }
    }

    return buttons;
  }

  action_t action_context::has_action(mousebtn btn, int x) const {
    return get_actions(x)[btn];
  }

  string action_context::get_action(action_t id) const {
    assert(id >= 0 && (unsigned)id < num_actions());

    return m_action_blocks[id].cmd;
  }

  bool action_context::has_double_click() const {
    for (auto&& a : m_action_blocks) {
      if (a.button == mousebtn::DOUBLE_LEFT || a.button == mousebtn::DOUBLE_MIDDLE ||
          a.button == mousebtn::DOUBLE_RIGHT) {
        return true;
      }
    }

    return false;
  }

  size_t action_context::num_actions() const {
    return m_action_blocks.size();
  }

  size_t action_context::num_unclosed() const {
    size_t num = 0;

    for (const auto& a : m_action_blocks) {
      if (a.is_open) {
        num++;
      }
    }

    return num;
  }

  const std::vector<action_block>& action_context::get_blocks() const {
    return m_action_blocks;
  }
} // namespace tags

POLYBAR_NS_END

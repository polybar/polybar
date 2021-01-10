#pragma once
#include <map>

#include "common.hpp"
#include "tags/context.hpp"
POLYBAR_NS

class renderer_interface {
 public:
  renderer_interface(tags::action_context& action_ctxt) : m_action_ctxt(action_ctxt){};

  virtual void render_offset(const tags::context& ctxt, int pixels) = 0;
  virtual void render_text(const tags::context& ctxt, const string&& str) = 0;
  virtual void change_alignment(const tags::context& ctxt) = 0;

  virtual void action_open(const tags::context& ctxt, mousebtn btn, tags::action_t id) = 0;
  virtual void action_close(const tags::context& ctxt, tags::action_t id) = 0;

 protected:
  /**
   * Stores information about actions in the current action cycle.
   *
   * The renderer is only responsible for updating the start and end positions
   * of each action block.
   */
  tags::action_context& m_action_ctxt;
};

POLYBAR_NS_END

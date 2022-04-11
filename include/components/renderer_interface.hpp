#pragma once
#include <map>

#include "common.hpp"
#include "tags/action_context.hpp"
#include "tags/context.hpp"
POLYBAR_NS

class renderer_interface {
 public:
  renderer_interface(const tags::action_context& action_ctxt) : m_action_ctxt(action_ctxt){};

  virtual void render_offset(const tags::context& ctxt, const extent_val offset) = 0;
  virtual void render_text(const tags::context& ctxt, const string&& str) = 0;
  virtual void change_alignment(const tags::context& ctxt) = 0;

  /**
   * Get the current x-coordinate of the renderer.
   *
   * This position denotes the coordinate where the next thing will be rendered.
   * It is relative to the start of the current alignment because the absolute
   * positions may not be known until after the renderer has finished.
   */
  virtual double get_x(const tags::context& ctxt) const = 0;

  /**
   * Get the absolute x-position of the start of an alignment block.
   *
   * The position is absolute in terms of the bar window.
   *
   * Only call this after all the rendering is finished as these values change
   * when new things are rendered.
   */
  virtual double get_alignment_start(const alignment align) const = 0;

  virtual void apply_tray_position(const tags::context& context) = 0;

 protected:
  /**
   * Stores information about actions in the current render cycle.
   */
  const tags::action_context& m_action_ctxt;
};

POLYBAR_NS_END

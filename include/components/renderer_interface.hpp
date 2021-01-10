#pragma once
#include <map>

#include "common.hpp"
#include "tags/context.hpp"
POLYBAR_NS

class renderer_interface {
 public:
  virtual void render_offset(const tags::context& ctxt, int pixels) = 0;
  virtual void render_text(const tags::context& ctxt, const string&& str) = 0;
  virtual void change_alignment(const tags::context& ctxt) = 0;

  virtual void action_open(const tags::context& ctxt, mousebtn btn, tags::action_t id) = 0;
  virtual void action_close(const tags::context& ctxt, tags::action_t id) = 0;

  virtual std::map<mousebtn, tags::action_t> get_actions(int x) = 0;
  virtual tags::action_t get_action(mousebtn btn, int x) = 0;
};

POLYBAR_NS_END

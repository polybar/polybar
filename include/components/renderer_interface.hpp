#pragma once
#include "common.hpp"
#include "tags/context.hpp"
POLYBAR_NS

class renderer_interface {
 public:
  virtual void render_offset(const tags::context& ctxt, int pixels) = 0;
  virtual void render_text(const tags::context& ctxt, const string&& str) = 0;
};

POLYBAR_NS_END

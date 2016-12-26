#pragma once

#include <map>

#include <X11/Xft/Xft.h>

#include "common.hpp"
#include "utils/color.hpp"

POLYBAR_NS

class color {
 public:
  explicit color(string hex);

  string source() const;

  operator XRenderColor() const;
  operator string() const;

  explicit operator uint32_t();
  operator uint32_t() const;

  static const color& parse(string input, const color& fallback);
  static const color& parse(string input);

 protected:
  uint32_t m_value;
  uint32_t m_color;
  string m_source;
};

extern color g_colorempty;
extern color g_colorblack;
extern color g_colorwhite;

POLYBAR_NS_END

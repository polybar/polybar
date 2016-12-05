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
  operator uint32_t() const;

  static color parse(string input, color fallback);
  static color parse(string input);

 protected:
  uint32_t m_value;
  uint32_t m_color;
  string m_source;
};

extern std::map<string, class color> g_colorstore;
extern color g_colorempty;
extern color g_colorblack;
extern color g_colorwhite;

POLYBAR_NS_END

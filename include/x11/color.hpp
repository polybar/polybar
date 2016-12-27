#pragma once

#include <map>

#include <X11/Xft/Xft.h>
#include <unordered_map>

#include "common.hpp"
#include "utils/color.hpp"
#include "utils/concurrency.hpp"

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

extern mutex_wrapper<std::unordered_map<string, color>> g_colorstore;

extern const color& g_colorempty;
extern const color& g_colorblack;
extern const color& g_colorwhite;

POLYBAR_NS_END

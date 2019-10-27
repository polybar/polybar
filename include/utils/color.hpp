#pragma once

#include <cstdlib>

#include "common.hpp"

POLYBAR_NS

struct rgba {
  /**
   * Color value in the form ARGB or A000 depending on the type
   */
  uint32_t m_value;

  enum color_type { NONE, ARGB, ALPHA_ONLY } m_type{NONE};

  explicit rgba();
  explicit rgba(uint32_t value, color_type type = ARGB);
  explicit rgba(string hex);

  operator string() const;
  operator uint32_t() const;
  bool operator==(const rgba& other) const;

  double a() const;
  double r() const;
  double g() const;
  double b() const;

  uint8_t a_int() const;
  uint8_t r_int() const;
  uint8_t g_int() const;
  uint8_t b_int() const;

  bool has_color() const;
};

namespace color_util {
  string simplify_hex(string hex);
}  // namespace color_util

POLYBAR_NS_END

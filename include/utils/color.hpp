#pragma once

#include <cstdlib>
#include <cstdint>

#include "common.hpp"

POLYBAR_NS

/**
 * Represents immutable 32-bit color values.
 */
class rgba {
 public:
  enum class type { NONE = 0, ARGB, ALPHA_ONLY };

  explicit rgba();
  explicit rgba(uint32_t value, type type = type::ARGB);
  explicit rgba(string hex);

  operator string() const;
  operator uint32_t() const;
  operator bool() const;
  bool operator==(const rgba& other) const;
  bool operator!=(const rgba& other) const;

  uint32_t value() const;
  type get_type() const;

  double alpha_d() const;
  double red_d() const;
  double green_d() const;
  double blue_d() const;

  uint8_t alpha_i() const;
  uint8_t red_i() const;
  uint8_t green_i() const;
  uint8_t blue_i() const;

  bool has_color() const;
  bool is_transparent() const;
  rgba apply_alpha_to(rgba other) const;
  rgba try_apply_alpha_to(rgba other) const;

 private:
  /**
   * Color value in the form ARGB or A000 depending on the type
   *
   * Cannot be const because we have to assign to it in the constructor and initializer lists are not possible.
   */
  uint32_t m_value;

  /**
   * NONE marks this instance as invalid. If such a color is encountered, it
   * should be treated as if no color was set.
   *
   * ALPHA_ONLY is used for color strings that only have an alpha channel (#AA)
   * these kinds should be combined with another color that has RGB channels
   * before they are used to render anything.
   *
   * Cannot be const because we have to assign to it in the constructor and initializer lists are not possible.
   */
  enum type m_type { type::NONE };
};

namespace color_util {
  string simplify_hex(string hex);
}  // namespace color_util

POLYBAR_NS_END

#pragma once

#include "common.hpp"

POLYBAR_NS

enum class alignment { NONE = 0, LEFT, CENTER, RIGHT };

enum class attribute { NONE = 0, UNDERLINE, OVERLINE };

enum class attr_activation { NONE, ON, OFF, TOGGLE };

enum class syntaxtag {
  A,  // mouse action
  B,  // background color
  F,  // foreground color
  T,  // font index
  O,  // pixel offset
  R,  // flip colors
  o,  // overline color
  u,  // underline color
  P,  // Polybar control tag
};

/**
 * Values for polybar control tags
 *
 * %{P...} tags are tags for internal polybar control commands, they are not
 * part of the public interface
 */
enum class controltag {
  NONE = 0,
  R,  // Reset all open tags (B, F, T, o, u). Used at module edges
};

POLYBAR_NS_END

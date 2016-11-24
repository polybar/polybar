#pragma once

#include "common.hpp"
#include "x11/color.hpp"
#include "x11/randr.hpp"

POLYBAR_NS

enum class edge : uint8_t { NONE = 0U, TOP, BOTTOM, LEFT, RIGHT, ALL };

enum class alignment : uint8_t { NONE = 0U, LEFT, CENTER, RIGHT };

enum class attribute : uint8_t { NONE = 0U, UNDERLINE, OVERLINE };

enum class syntaxtag : uint8_t {
  NONE = 0U,
  A,  // mouse action
  B,  // background color
  F,  // foreground color
  T,  // font index
  O,  // pixel offset
  R,  // flip colors
  o,  // overline color
  u,  // underline color
};

enum class mousebtn : uint8_t { NONE = 0U, LEFT, MIDDLE, RIGHT, SCROLL_UP, SCROLL_DOWN };

enum class strut : uint16_t {
  LEFT = 0U,
  RIGHT,
  TOP,
  BOTTOM,
  LEFT_START_Y,
  LEFT_END_Y,
  RIGHT_START_Y,
  RIGHT_END_Y,
  TOP_START_X,
  TOP_END_X,
  BOTTOM_START_X,
  BOTTOM_END_X,
};

struct position {
  int16_t x{0};
  int16_t y{0};
};

struct size {
  uint16_t w{1U};
  uint16_t h{1U};
};

struct side_values {
  uint16_t left{0U};
  uint16_t right{0U};
};

struct edge_values {
  uint16_t left{0U};
  uint16_t right{0U};
  uint16_t top{0U};
  uint16_t bottom{0U};
};

struct border_settings {
  uint32_t color{0xFF000000};
  uint16_t size{0U};
};

struct line_settings {
  uint32_t color{0xFF000000};
  uint16_t size{0U};
};

struct bar_settings {
  monitor_t monitor;
  edge origin{edge::TOP};
  struct size size {
    1U, 1U
  };
  position pos{0, 0};
  position offset{0, 0};
  position center{0, 0};
  side_values padding{0U, 0U};
  side_values margin{0U, 0U};
  side_values module_margin{0U, 2U};
  edge_values strut{0U, 0U, 0U, 0U};

  uint32_t background{0xFFFFFFFF};
  uint32_t foreground{0xFF000000};

  line_settings underline;
  line_settings overline;

  map<edge, border_settings> borders;

  uint8_t spacing{1U};
  string separator;

  string wmname;
  string locale;

  bool force_docking{false};

  const xcb_rectangle_t inner_area(bool local_coords = false) const {
    xcb_rectangle_t rect{pos.x, pos.y, size.w, size.h};
    if (local_coords) {
      rect.x = 0;
      rect.y = 0;
    }
    rect.y += borders.at(edge::TOP).size;
    rect.height -= borders.at(edge::TOP).size;
    rect.height -= borders.at(edge::BOTTOM).size;
    rect.x += borders.at(edge::LEFT).size;
    rect.width -= borders.at(edge::LEFT).size;
    rect.width -= borders.at(edge::RIGHT).size;
    return rect;
  }
};

struct action_block {
  alignment align{alignment::NONE};
  int16_t start_x{0};
  int16_t end_x{0};
  mousebtn button{mousebtn::NONE};
  string command;
  bool active{true};
#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
  xcb_window_t hint;
#endif
};

POLYBAR_NS_END

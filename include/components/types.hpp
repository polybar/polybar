#pragma once

#include "common.hpp"
#include "x11/color.hpp"
#include "x11/randr.hpp"

POLYBAR_NS

enum class edge : uint8_t { NONE = 0, TOP, BOTTOM, LEFT, RIGHT, ALL };
enum class alignment : uint8_t { NONE = 0, LEFT, CENTER, RIGHT };
enum class syntaxtag : uint8_t { NONE = 0, A, B, F, T, U, O, R, o, u };
enum class attribute : uint8_t { NONE = 0, o = 1 << 0, u = 1 << 1 };
enum class mousebtn : uint8_t { NONE = 0, LEFT, MIDDLE, RIGHT, SCROLL_UP, SCROLL_DOWN };
enum class gc : uint8_t { NONE = 0, BG, FG, OL, UL, BT, BB, BL, BR };
enum class strut : uint16_t {
  LEFT = 0,
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
  uint16_t w{0};
  uint16_t h{0};
};

struct side_values {
  uint16_t left{0};
  uint16_t right{0};
};

struct edge_values {
  uint16_t left{0};
  uint16_t right{0};
  uint16_t top{0};
  uint16_t bottom{0};
};

struct border_settings {
  uint32_t color{0xFF000000};
  uint16_t size{0};
};

struct bar_settings {
  monitor_t monitor;

  edge origin{edge::BOTTOM};

  size size{0, 0};
  position pos{0, 0};
  position offset{0, 0};
  position center{0, 0};
  side_values padding{0, 0};
  side_values margin{0, 0};
  side_values module_margin{0, 2};
  edge_values strut{0, 0, 0, 0};

  uint32_t background{0xFFFFFFFF};
  uint32_t foreground{0xFF0000FF};
  uint32_t linecolor{0xFF000000};

  map<edge, border_settings> borders;

  int8_t lineheight{0};
  int8_t spacing{1};
  string separator;

  string wmname;
  string locale;

  bool force_docking{false};
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

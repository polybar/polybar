#pragma once

#include <xcb/xcb.h>

#include <string>
#include <unordered_map>

#include "common.hpp"

POLYBAR_NS

// fwd {{{
struct randr_output;
using monitor_t = shared_ptr<randr_output>;
// }}}

struct enum_hash {
  template <typename T>
  inline typename std::enable_if<std::is_enum<T>::value, size_t>::type operator()(T const value) const {
    return static_cast<size_t>(value);
  }
};

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

enum class mousebtn : uint8_t {
  NONE = 0U,
  LEFT,
  MIDDLE,
  RIGHT,
  SCROLL_UP,
  SCROLL_DOWN,
  DOUBLE_LEFT,
  DOUBLE_MIDDLE,
  DOUBLE_RIGHT
};

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

struct action {
  mousebtn button{mousebtn::NONE};
  string command{};
};

struct action_block : public action {
  alignment align{alignment::NONE};
  double start_x{0.0};
  double end_x{0.0};
  bool active{true};

  uint16_t width() const {
    return static_cast<uint16_t>(end_x - start_x + 0.5);
  }

  bool test(int16_t point) const {
    return static_cast<int16_t>(start_x) < point && static_cast<int16_t>(end_x) >= point;
  }
};

struct bar_settings {
  explicit bar_settings() = default;
  bar_settings(const bar_settings& other) = default;

  xcb_window_t window{XCB_NONE};
  monitor_t monitor{};
  edge origin{edge::TOP};
  struct size size {
    1U, 1U
  };
  position pos{0, 0};
  position offset{0, 0};
  position center{0, 0};
  side_values padding{0U, 0U};
  side_values margin{0U, 0U};
  side_values module_margin{0U, 0U};
  edge_values strut{0U, 0U, 0U, 0U};

  uint32_t background{0xFF000000};
  uint32_t foreground{0xFFFFFFFF};
  vector<uint32_t> background_steps;

  line_settings underline{};
  line_settings overline{};

  std::unordered_map<edge, border_settings, enum_hash> borders{};

  uint8_t spacing{0};
  string separator{};

  string wmname{};
  string locale{};

  bool override_redirect{false};

  vector<action> actions{};

  bool dimmed{false};
  double dimvalue{1.0};

  bool shaded{false};
  struct size shade_size {
    1U, 1U
  };
  position shade_pos{1U, 1U};

  const xcb_rectangle_t inner_area(bool abspos = false) const {
    xcb_rectangle_t rect{0, 0, size.w, size.h};

    if (abspos) {
      rect.x = pos.x;
      rect.y = pos.y;
    }
    if (borders.find(edge::TOP) != borders.end()) {
      rect.y += borders.at(edge::TOP).size;
      rect.height -= borders.at(edge::TOP).size;
    }
    if (borders.find(edge::BOTTOM) != borders.end()) {
      rect.height -= borders.at(edge::BOTTOM).size;
    }
    if (borders.find(edge::LEFT) != borders.end()) {
      rect.x += borders.at(edge::LEFT).size;
      rect.width -= borders.at(edge::LEFT).size;
    }
    if (borders.find(edge::RIGHT) != borders.end()) {
      rect.width -= borders.at(edge::RIGHT).size;
    }
    return rect;
  }
};

struct event_timer {
  xcb_timestamp_t event{0L};
  xcb_timestamp_t offset{1L};

  bool allow(xcb_timestamp_t time) {
    bool pass = time >= event + offset;
    event = time;
    return pass;
  };

  bool deny(xcb_timestamp_t time) {
    return !allow(time);
  };
};

POLYBAR_NS_END

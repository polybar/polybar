#pragma once

#include <xcb/xcb.h>

#include <string>
#include <unordered_map>

#include "common.hpp"
#include "utils/color.hpp"

POLYBAR_NS

// fwd {{{
struct randr_output;
using monitor_t = shared_ptr<randr_output>;

namespace drawtypes {
  class label;
}

using label_t = shared_ptr<drawtypes::label>;
// }}}

struct enum_hash {
  template <typename T>
  inline typename std::enable_if<std::is_enum<T>::value, size_t>::type operator()(T const value) const {
    return static_cast<size_t>(value);
  }
};

enum class edge { NONE = 0, TOP, BOTTOM, LEFT, RIGHT, ALL };

enum class alignment { NONE = 0, LEFT, CENTER, RIGHT };

enum class mousebtn {
  NONE = 0,
  LEFT,
  MIDDLE,
  RIGHT,
  SCROLL_UP,
  SCROLL_DOWN,
  DOUBLE_LEFT,
  DOUBLE_MIDDLE,
  DOUBLE_RIGHT,
  // Terminator value, do not use
  BTN_COUNT,
};

enum class strut {
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
  int x{0};
  int y{0};
};

struct size {
  unsigned int w{1U};
  unsigned int h{1U};
};

struct side_values {
  unsigned int left{0U};
  unsigned int right{0U};
};

struct edge_values {
  unsigned int left{0U};
  unsigned int right{0U};
  unsigned int top{0U};
  unsigned int bottom{0U};
};

struct radius {
  double top_left{0.0};
  double top_right{0.0};
  double bottom_left{0.0};
  double bottom_right{0.0};

  operator bool() const {
    return top_left != 0.0 || top_right != 0.0 || bottom_left != 0.0 || bottom_right != 0.0;
  }
};

struct border_settings {
  rgba color{0xFF000000};
  unsigned int size{0U};
};

struct line_settings {
  rgba color{0xFF000000};
  unsigned int size{0U};
};

struct action {
  mousebtn button{mousebtn::NONE};
  string command{};
};

struct bar_settings {
  explicit bar_settings() = default;
  bar_settings(const bar_settings& other) = default;

  xcb_window_t window{XCB_NONE};
  monitor_t monitor{};
  bool monitor_strict{false};
  bool monitor_exact{true};
  edge origin{edge::TOP};
  struct size size {
    1U, 1U
  };
  position pos{0, 0};
  position offset{0, 0};
  side_values padding{0U, 0U};
  side_values margin{0U, 0U};
  side_values module_margin{0U, 0U};
  edge_values strut{0U, 0U, 0U, 0U};

  rgba background{0xFF000000};
  rgba foreground{0xFFFFFFFF};
  vector<rgba> background_steps;

  line_settings underline{};
  line_settings overline{};

  std::unordered_map<edge, border_settings, enum_hash> borders{};

  struct radius radius {};
  int spacing{0};
  label_t separator{};

  string wmname{};
  string locale{};

  bool override_redirect{false};

  string cursor{};
  string cursor_click{};
  string cursor_scroll{};

  vector<action> actions{};

  bool dimmed{false};
  double dimvalue{1.0};

  bool shaded{false};
  struct size shade_size {
    1U, 1U
  };
  position shade_pos{1U, 1U};

  const xcb_rectangle_t inner_area(bool abspos = false) const {
    xcb_rectangle_t rect = this->outer_area(abspos);

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

  const xcb_rectangle_t outer_area(bool abspos = false) const {
    xcb_rectangle_t rect{0, 0, 0, 0};
    rect.width += size.w;
    rect.height += size.h;

    if (abspos) {
      rect.x = pos.x;
      rect.y = pos.y;
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

enum class output_policy {
  REDIRECTED,
  IGNORED,
};

POLYBAR_NS_END

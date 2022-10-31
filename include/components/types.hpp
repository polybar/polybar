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

static inline mousebtn mousebtn_get_double(mousebtn btn) {
  switch (btn) {
    case mousebtn::LEFT:
      return mousebtn::DOUBLE_LEFT;
    case mousebtn::MIDDLE:
      return mousebtn::DOUBLE_MIDDLE;
    case mousebtn::RIGHT:
      return mousebtn::DOUBLE_RIGHT;
    default:
      return mousebtn::NONE;
  }
}

/**
 * Order of values for _NET_WM_STRUT_PARTIAL
 *
 * https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm45381391268672
 */
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

  bool operator==(const position& other) {
    return this->x == other.x && this->y == other.y;
  }
};

struct size {
  unsigned w{1U};
  unsigned h{1U};
};

enum class spacing_type { SPACE, POINT, PIXEL };

enum class extent_type { POINT, PIXEL };

struct spacing_val {
  spacing_type type{spacing_type::SPACE};
  /**
   * Numerical spacing value. Is truncated to an integer for pixels and spaces.
   * Must be non-negative.
   */
  float value{0};

  /**
   * Any non-positive number is interpreted as no spacing.
   */
  operator bool() const {
    return value > 0;
  }
};

static constexpr spacing_val ZERO_SPACE = {spacing_type::SPACE, 0};

/*
 * Defines the signed length of something as either a number of pixels or points.
 *
 * Used for widths, heights, and offsets
 */
struct extent_val {
  extent_type type{extent_type::PIXEL};
  float value{0};

  operator bool() const {
    return value != 0;
  }
};

static constexpr extent_val ZERO_PX_EXTENT = {extent_type::PIXEL, 0};

struct side_values {
  spacing_val left{ZERO_SPACE};
  spacing_val right{ZERO_SPACE};
};

struct percentage_with_offset {
  double percentage{0};
  extent_val offset{ZERO_PX_EXTENT};
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

/**
 * Settings specific to the X window system.
 */
struct x_settings {
  xcb_window_t window{XCB_NONE};
  xcb_visualtype_t* visual{nullptr};
  int depth{-1};
};

struct bar_settings {
  explicit bar_settings() = default;
  bar_settings(const bar_settings& other) = default;

  x_settings x_data;

  monitor_t monitor{};
  bool monitor_strict{false};
  bool monitor_exact{true};
  bool bottom{false};
  struct size size {
    1U, 1U
  };

  double dpi_x{0.};
  double dpi_y{0.};

  position pos{0, 0};
  position offset{0, 0};
  side_values padding{ZERO_SPACE, ZERO_SPACE};
  side_values module_margin{ZERO_SPACE, ZERO_SPACE};
  bool struts{true};
  struct {
    int top;
    int bottom;
  } strut{0, 0};

  rgba background{0xFF000000};
  rgba foreground{0xFFFFFFFF};
  vector<rgba> background_steps;

  line_settings underline{};
  line_settings overline{};

  std::unordered_map<edge, border_settings, enum_hash> borders{};

  struct radius radius {};
  /**
   * TODO deprecated
   */
  spacing_val spacing{ZERO_SPACE};
  label_t separator{};

  string wmname{};
  string locale{};

  bool override_redirect{false};

  int double_click_interval{400};

  /**
   * Name of cursor to use for clickable areas
   */
  string cursor_click{};

  /**
   * Name of cursor to use for scrollable areas
   */
  string cursor_scroll{};

  vector<action> actions{};

  bool dimmed{false};
  double dimvalue{1.0};

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

POLYBAR_NS_END

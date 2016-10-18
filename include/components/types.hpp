#pragma once

#include "common.hpp"
#include "components/x11/color.hpp"
#include "components/x11/randr.hpp"

LEMONBUDDY_NS

enum class border { NONE = 0, TOP, BOTTOM, LEFT, RIGHT, ALL };
enum class alignment { NONE = 0, LEFT, CENTER, RIGHT };
enum class syntaxtag { NONE = 0, A, B, F, T, U, O, R, o, u };
enum class attribute { NONE = 0, o = 2, u = 4 };
enum class mousebtn { NONE = 0, LEFT, MIDDLE, RIGHT, SCROLL_UP, SCROLL_DOWN };
enum class gc { NONE = 0, BG, FG, OL, UL, BT, BB, BL, BR };

struct bar_settings {
  bar_settings() = default;

  string locale;

  int16_t x{0};
  int16_t y{0};
  uint16_t width{0};
  uint16_t height{0};

  int16_t offset_y{0};
  int16_t offset_x{0};

  uint16_t padding_left{0};
  uint16_t padding_right{0};

  int16_t module_margin_left{0};
  int16_t module_margin_right{2};

  int16_t lineheight{0};
  int16_t spacing{1};
  string separator;

  color background{g_colorwhite};
  color foreground{g_colorblack};
  color linecolor{g_colorblack};

  alignment align{alignment::RIGHT};

  bool bottom{false};
  bool dock{false};

  monitor_t monitor;
  string wmname;

  int16_t vertical_mid{0};

  string geom() {
    char buffer[32]{
        '\0',
    };
    snprintf(buffer, sizeof(buffer), "%dx%d+%d+%d", width, height, x, y);
    return string{*buffer};
  };
};

struct tray_settings {
  tray_settings() = default;

  tray_settings& operator=(tray_settings& o) {
    background = o.background;
    align = o.align;
    orig_x = o.orig_x;
    orig_y = o.orig_y;
    width = o.width;
    height = o.height;
    spacing = o.spacing;
    slots = o.slots;
    sibling = o.sibling;
    return *this;
  }

  uint32_t background;
  alignment align{alignment::NONE};
  int16_t orig_x{0};
  int16_t orig_y{0};
  uint16_t width{0};
  uint16_t height{0};
  uint16_t spacing{0};
  uint16_t slots{0};
  uint32_t sibling;
};

struct border_settings {
  border_settings() = default;
  lemonbuddy::color color{g_colorblack};
  uint16_t size{0};
};

struct action_block {
  action_block() = default;
  mousebtn button{mousebtn::NONE};
  string command;
  int16_t start_x{0};
  int16_t end_x{0};
  alignment align;
  bool active{true};
#if DEBUG and DRAW_CLICKABLE_AREA_HINTS
  xcb_window_t clickable_area;
#endif
};

struct wmsettings_bspwm {
};

LEMONBUDDY_NS_END

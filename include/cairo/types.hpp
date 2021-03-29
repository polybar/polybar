#pragma once

#include <cairo/cairo.h>

#include "common.hpp"
#include "components/types.hpp"

POLYBAR_NS

enum class alignment;

namespace cairo {
  struct point {
    double x;
    double y;
  };
  struct abspos {
    double x;
    double y;
    bool clear{true};
  };
  struct relpos {
    double x;
    double y;
  };
  struct rect {
    double x;
    double y;
    double w;
    double h;
  };
  struct line {
    double x1;
    double y1;
    double x2;
    double y2;
    double w;
  };
  struct translate {
    double x;
    double y;
  };
  struct linear_gradient {
    double x1;
    double y1;
    double x2;
    double y2;
    vector<rgba> steps;
  };
  struct rounded_corners {
    double x;
    double y;
    double w;
    double h;
    struct radius radius;
  };
  struct circle_segment {
    double x;
    double y;
    double w;
    double angle_from;
    double angle_to;
    double radius;
  };
  struct textblock {
    alignment align;
    string contents;
    int font;
    rgba bg{};
    cairo_operator_t bg_operator;
    rect bg_rect;
    double* x_advance;
    double* y_advance;
  };
}  // namespace cairo

POLYBAR_NS_END

#pragma once

#include "common.hpp"

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
  struct displace {
    double x;
    double y;
    double w;
    double h;
    double dx;
    double dy;
  };
  struct linear_gradient {
    double x1;
    double y1;
    double x2;
    double y2;
    vector<unsigned int> steps;
  };
  struct rounded_corners {
    double x;
    double y;
    double w;
    double h;
    double radius;
  };
  struct textblock {
    alignment align;
    string contents;
    int fontindex;
    unsigned int bg;
    int bg_operator;
    rect bg_rect;
    double *x_advance;
    double *y_advance;
  };
}

POLYBAR_NS_END

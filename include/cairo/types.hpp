#pragma once

#include "common.hpp"

POLYBAR_NS

enum class alignment;

namespace cairo {
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

  struct linear_gradient {
    double x0;
    double y0;
    double x1;
    double y1;
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
  };
}

POLYBAR_NS_END

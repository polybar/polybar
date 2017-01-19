#pragma once

#include "common.hpp"

POLYBAR_NS

namespace cairo {
  struct abspos {
    double x;
    double y;
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

  struct textblock {
    string contents;
    unsigned char fontindex;
  };
}

POLYBAR_NS_END

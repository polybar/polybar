#pragma once

#include <sstream>
#include <memory>
#include <vector>

namespace xlib
{
  struct Monitor
  {
    std::string name;
    int index = 0;
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;

    std::string str() {
      return this->name + ": "+ this->geom();
    }

    std::string geom() {
      return std::to_string(width) +"x"+ std::to_string(height) +"+"+
        std::to_string(x) +"+"+ std::to_string(y);
    }
  };

  // std::unique_ptr<Monitor> get_monitor(std::string monitor_name);

  std::vector<std::unique_ptr<Monitor>> get_sorted_monitorlist();
}

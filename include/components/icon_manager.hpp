#pragma once

#include "common.hpp"
#include "errors.hpp"

#include <mutex>

POLYBAR_NS

struct icon_data;
namespace modules {
  struct module_interface;
}
namespace cairo {
  class icon_surface;
}
using icon_surface_t = shared_ptr<cairo::icon_surface>;


class icon_manager {
 public:
  explicit icon_manager() {};
  void add_icon(icon_surface_t surface, uint64_t id, modules::module_interface* module);
  icon_surface_t get_icon(uint64_t id);
  void clear_icons(modules::module_interface* module);

 private:
  vector<icon_data> m_icons{};
  std::mutex m_mutex{};
  //TODO provide default missing icon
  icon_surface_t m_missing_icon{};

};

POLYBAR_NS_END

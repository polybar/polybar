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
  class surface;
}
using surface_t = shared_ptr<cairo::surface>;


class icon_manager {
 public:
  explicit icon_manager() {};
  void add_icon(surface_t surface, uint64_t id, modules::module_interface* module);
  surface_t get_icon(uint64_t id);
  void clear_icons(modules::module_interface* module);

 private:
  vector<icon_data> m_icons{};
  std::mutex m_mutex{};
  //TODO provide default missing icon
  surface_t m_missing_icon{};

};

POLYBAR_NS_END

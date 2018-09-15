#pragma once

#include "common.hpp"
#include "errors.hpp"

#include <mutex>

POLYBAR_NS

//fwd
struct icon_data;
namespace modules {
  struct module_interface;
}

class icon_manager {
 public:
  explicit icon_manager() {};
  void add_icon(vector<unsigned char> buf, uint64_t id, modules::module_interface* module);
  //vector<unsigned char>& get_icon(uint64_t id, modules::module_interface* module);
  vector<unsigned char>& get_icon(uint64_t id);
  void clear_icons(modules::module_interface* module);
 private:
  vector<icon_data> m_icons{};
  std::mutex m_mutex{};
  //TODO provide default missing icon
  vector<unsigned char> m_missing_icon{};
};

POLYBAR_NS_END

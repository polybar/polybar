#include "utils/restack.hpp"

#include <algorithm>

POLYBAR_NS

namespace restack {
  wm_restacker::wm_restacker(std::string&& name) {
    get_restacker_map().emplace(move(name), *this);
  }

  restacker_map_t& get_restacker_map() {
    static restacker_map_t restacker_map;
    return restacker_map;
  }

  wm_restacker* get_restacker(const std::string& wm_name) {
    auto it = get_restacker_map().find(wm_name);

    if (it == get_restacker_map().end()) {
      return nullptr;
    }

    return &(it->second);
  }
}  // namespace restack

POLYBAR_NS_END

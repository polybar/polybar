#include "utils/restack.hpp"

#include <algorithm>

POLYBAR_NS

namespace restack {
  namespace {
    using restacker_map_t = std::unordered_map<std::string, wm_restacker&>;

    restacker_map_t& get_restacker_map() {
      static restacker_map_t restacker_map;
      return restacker_map;
    }
  }

  wm_restacker::wm_restacker(std::string&& name) {
    get_restacker_map().emplace(move(name), *this);
  }

  wm_restacker* get_restacker(const std::string& wm_name) {
    const auto& map = get_restacker_map();
    auto it = map.find(wm_name);

    if (it == map.end()) {
      return nullptr;
    }

    return &(it->second);
  }
}  // namespace restack

POLYBAR_NS_END

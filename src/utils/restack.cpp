#include "utils/restack.hpp"

POLYBAR_NS

namespace restack {

  restacker_map& get_restacker_map() {
    static restacker_map rmap;
    return rmap;
  }

  const wm_restacker* get_restacker(const std::string& name) {
    const auto& rmap = get_restacker_map();
    auto it = rmap.find(name);
    if (it != rmap.end()) {
      return &it->second();
    } else {
      return nullptr;
    }
  }

}  // namespace restack

POLYBAR_NS_END

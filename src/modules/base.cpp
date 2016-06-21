#include "lemonbuddy.hpp"
#include "registry.hpp"
#include "modules/base.hpp"
#include "utils/config.hpp"
#include "utils/string.hpp"

namespace modules
{
  void broadcast_module_update(std::string module_name)
  {
    log_trace("Broadcasting module update for => "+ module_name);
    get_registry()->notify(module_name);
  }

  std::string get_tag_name(std::string tag) {
    return tag.length() < 2 ? "" : tag.substr(1, tag.length()-2);
  }
}

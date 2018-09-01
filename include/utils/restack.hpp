#pragma once

#include <unordered_map>

#include "common.hpp"
#include "components/types.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

class connection;
class logger;

namespace restack {

  struct wm_restacker {
    virtual void operator()(connection& conn, const bar_settings& opts, const logger& log) const = 0;
    virtual ~wm_restacker() = default;
  };

  using factory_function = const wm_restacker& (*)();
  using restacker_map = std::unordered_map<std::string, factory_function>;

  restacker_map& get_restacker_map();

  const wm_restacker* get_restacker(const std::string& name);

  template <class Restacker>
  struct restacker_registration {
    restacker_registration(std::string name, factory_function ff) {
      get_restacker_map()[std::move(name)] = ff;
    }
  };

#define POLYBAR_RESTACKER(RESTACKER_TYPE, RESTACKER_NAME) \
  const wm_restacker& get_##RESTACKER_TYPE() {\
    static RESTACKER_TYPE local_restacker;\
    return local_restacker;\
  }\
  restacker_registration<RESTACKER_TYPE> RESTACKER_TYPE##_registration(RESTACKER_NAME, &get_##RESTACKER_TYPE)

}  // namespace restack

POLYBAR_NS_END

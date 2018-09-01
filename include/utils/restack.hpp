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
  };

  using restacker_map = std::unordered_map<std::string, std::unique_ptr<wm_restacker>>;

  restacker_map& get_restacker_map();

  const wm_restacker* get_restacker(const std::string& name);

  template <class Restacker>
  struct restacker_registration {
    restacker_registration(std::string name) {
      get_restacker_map()[std::move(name)] = std::make_unique<Restacker>();
    }
  };

#define POLYBAR_RESTACKER(RESTACKER_TYPE, RESTACKER_NAME) \
  restacker_registration<RESTACKER_TYPE> RESTACKER_TYPE##_registration(RESTACKER_NAME)

}  // namespace restack

POLYBAR_NS_END

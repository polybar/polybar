#pragma once

#include <array>
#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

class plugin_handle {
 public:
  plugin_handle(const char* libname);
  ~plugin_handle();

  plugin_handle(const plugin_handle&) = delete;
  plugin_handle& operator=(const plugin_handle&) = delete;

  plugin_handle(plugin_handle&&);
  plugin_handle& operator=(plugin_handle&&);

 private:
  void* m_handle = nullptr;
};

extern std::vector<const char*> plugin_names;

POLYBAR_NS_END

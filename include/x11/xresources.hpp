#pragma once

#include <X11/Xresource.h>

#include "common.hpp"

POLYBAR_NS

class xresource_manager {
 public:
  explicit xresource_manager();

  string get_string(string name, string fallback = "") const;
  float get_float(string name, float fallback = 0.0f) const;
  int get_int(string name, int fallback = 0) const;

 protected:
  string load_value(string key, string res_type, size_t n) const;

 private:
  char* m_manager = nullptr;
  XrmDatabase m_db;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = const xresource_manager&>
  di::injector<T> configure_xresource_manager() {
    auto instance = factory::generic_singleton<xresource_manager>();
    return di::make_injector(di::bind<>().to(instance));
  }
}

POLYBAR_NS_END

#pragma once

#include <X11/Xresource.h>

#include "common.hpp"

POLYBAR_NS

class xresource_manager {
 public:
  using make_type = const xresource_manager&;
  static make_type make();

  explicit xresource_manager();
  ~xresource_manager();

  string get_string(string name, string fallback = "") const;
  float get_float(string name, float fallback = 0.0f) const;
  int get_int(string name, int fallback = 0) const;

 protected:
  string load_value(const string& key, const string& res_type, size_t n) const;

 private:
  char* m_manager{nullptr};
  XrmDatabase m_db;
};

POLYBAR_NS_END

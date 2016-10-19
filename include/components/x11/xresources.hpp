#pragma once

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "common.hpp"
#include "components/x11/xlib.hpp"

LEMONBUDDY_NS

class xresource_manager {
 public:
  explicit xresource_manager() {
    XrmInitialize();

    if (xlib::get_display() == nullptr)
      return;
    if ((m_manager = XResourceManagerString(xlib::get_display())) == nullptr)
      return;
    if ((m_db = XrmGetStringDatabase(m_manager)) == nullptr)
      return;
  }

  /**
   * Configure injection module
   */
  template <typename T = const xresource_manager&>
  static di::injector<T> configure() {
    auto instance = factory::generic_singleton<xresource_manager>();
    return di::make_injector(di::bind<>().to(instance));
  }

  string get_string(string name) const {
    return load_value(name, "String", 64);
  }

  float get_float(string name) const {
    return std::strtof(load_value(name, "String", 64).c_str(), 0);
  }

  int get_int(string name) const {
    return std::atoi(load_value(name, "String", 64).c_str());
  }

 protected:
  string load_value(string key, string res_type, size_t n) const {
    if (m_manager == nullptr)
      return "";

    char* type = nullptr;
    XrmValue ret;
    XrmGetResource(m_db, key.c_str(), res_type.c_str(), &type, &ret);

    if (ret.addr != nullptr && !std::strncmp(res_type.c_str(), type, n)) {
      return {ret.addr};
    }

    return "";
  }

 private:
  char* m_manager = nullptr;
  XrmDatabase m_db;
};

LEMONBUDDY_NS_END

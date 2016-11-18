#include <X11/Xlib.h>

#include "x11/xlib.hpp"
#include "x11/xresources.hpp"

LEMONBUDDY_NS

xresource_manager::xresource_manager() {
  XrmInitialize();

  if (xlib::get_display() == nullptr)
    return;
  if ((m_manager = XResourceManagerString(xlib::get_display())) == nullptr)
    return;
  if ((m_db = XrmGetStringDatabase(m_manager)) == nullptr)
    return;
}

string xresource_manager::get_string(string name, string fallback) const {
  auto result = load_value(name, "String", 64);
  if (result.empty())
    return fallback;
  return result;
}

float xresource_manager::get_float(string name, float fallback) const {
  auto result = load_value(name, "String", 64);
  if (result.empty())
    return fallback;
  return strtof(result.c_str(), 0);
}

int xresource_manager::get_int(string name, int fallback) const {
  auto result = load_value(name, "String", 64);
  if (result.empty())
    return fallback;
  return atoi(result.c_str());
}

string xresource_manager::load_value(string key, string res_type, size_t n) const {
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

LEMONBUDDY_NS_END

#include <X11/Xlib.h>
#include <cstring>
#include <utility>

#include "utils/factory.hpp"
#include "x11/connection.hpp"
#include "x11/xresources.hpp"

POLYBAR_NS

/**
 * Create instance
 */
xresource_manager::make_type xresource_manager::make() {
  return static_cast<xresource_manager::make_type>(
      *factory_util::singleton<std::remove_reference_t<xresource_manager::make_type>>(connection::make()));
}

/**
 * Construct manager instance
 */
xresource_manager::xresource_manager(Display* dsp) : m_display(forward<decltype(dsp)>(dsp)) {
  XrmInitialize();

  if ((m_manager = XResourceManagerString(m_display)) != nullptr) {
    m_db = XrmGetStringDatabase(m_manager);
  }
}

/**
 * Deconstruct instance
 */
xresource_manager::~xresource_manager() {
  if (m_manager != nullptr) {
    XFree(m_manager);
  }
  if (m_db != nullptr) {
    XrmDestroyDatabase(m_db);
  }
}

/**
 * Get string value from the X resource db
 */
string xresource_manager::get_string(string name, string fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return result;
}

/**
 * Get float value from the X resource db
 */
float xresource_manager::get_float(string name, float fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return strtof(result.c_str(), nullptr);
}

/**
 * Get integer value from the X resource db
 */
int xresource_manager::get_int(string name, int fallback) const {
  auto result = load_value(move(name), "String", 64);
  if (result.empty()) {
    return fallback;
  }
  return atoi(result.c_str());
}

/**
 * Query the database for given key
 */
string xresource_manager::load_value(const string& key, const string& res_type, size_t n) const {
  if (m_manager == nullptr) {
    return "";
  }
  char* type = nullptr;
  XrmValue ret{};
  XrmGetResource(m_db, key.c_str(), res_type.c_str(), &type, &ret);

  if (ret.addr != nullptr && !std::strncmp(res_type.c_str(), type, n)) {
    return {ret.addr};
  }

  return "";
}

POLYBAR_NS_END

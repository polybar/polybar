#pragma once

#include "settings.hpp"

#if not WITH_XRM
#error "Not built with support for xcb-xrm..."
#endif

#include <xcb/xcb_xrm.h>

#include "common.hpp"
#include "errors.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"

POLYBAR_NS

DEFINE_ERROR(xresource_error);

class xresource_manager {
 public:
  explicit xresource_manager(xcb_connection_t* conn) : m_xrm(xcb_xrm_database_from_default(conn)) {
    if (m_xrm == nullptr) {
      throw application_error("xcb_xrm_database_from_default()");
    }
  }

  ~xresource_manager() {
    xcb_xrm_database_free(m_xrm);
  }

  operator bool() const {
    return m_xrm != nullptr;
  }

  template <typename T>
  T require(const char* name) const {
    char* result{nullptr};
    if (xcb_xrm_resource_get_string(m_xrm, string_util::replace(name, "*", ".").c_str(), nullptr, &result) == -1) {
      throw xresource_error(sstream() << "X resource \"" << name << "\" not found");
    } else if (result == nullptr) {
      throw xresource_error(sstream() << "X resource \"" << name << "\" not found");
    }
    return convert<T>(string(result));
  }

  template <typename T>
  T get(const char* name, const T& fallback) const {
    try {
      return convert<T>(require<T>(name));
    } catch (const xresource_error&) {
      return fallback;
    }
  }

 protected:
  template <typename T>
  T convert(string&& value) const;

 private:
  xcb_xrm_database_t* m_xrm;
};

POLYBAR_NS_END

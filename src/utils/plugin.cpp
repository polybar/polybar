#include <dlfcn.h>

#include "components/logger.hpp"
#include "errors.hpp"
#include "utils/plugin.hpp"

POLYBAR_NS

plugin_handle::plugin_handle(const char* libname) {
  m_handle = ::dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
  if (!m_handle)
    throw application_error(::dlerror());
}

plugin_handle::~plugin_handle() {
  if (m_handle) {
    ::dlclose(m_handle);
  }
}

plugin_handle::plugin_handle(plugin_handle&& ph) {
  m_handle = ph.m_handle;
  ph.m_handle = nullptr;
}

plugin_handle& plugin_handle::operator=(plugin_handle&& ph) {
  if (m_handle) {
    ::dlclose(m_handle);
  }
  m_handle = ph.m_handle;
  ph.m_handle = nullptr;
  return *this;
}

POLYBAR_NS_END

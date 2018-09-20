#include <dlfcn.h>

#include "components/logger.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/plugin.hpp"

POLYBAR_NS

// clang-format off
std::vector<const char*> plugin_names = {
#if ENABLE_I3
  "libpolybar-utils-i3.so",
#endif
#if ENABLE_ALSA
  "libpolybar-modules-alsa.so",
#endif
#if ENABLE_CURL
  "libpolybar-modules-github.so",
#endif
#if ENABLE_I3
  "libpolybar-modules-i3.so",
#endif
#if ENABLE_MPD
  "libpolybar-modules-mpd.so",
#endif
#if ENABLE_NETWORK
  "libpolybar-modules-network.so",
#endif
#if ENABLE_PLUSEAUDIO
  "libpolybar-modules-pulseaudio.so",
#endif
#if WITH_XKB
  "libpolybar-modules-xkeyboard.so",
#endif
  nullptr
};
// clang-format on

plugin_handle::plugin_handle(const char* libname) {
  m_handle = ::dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
  if (!m_handle) {
    throw application_error(::dlerror());
  }
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

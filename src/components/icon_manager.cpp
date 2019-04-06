#include <cairo.h>
#include "cairo/surface.hpp"
#include "components/icon_manager.hpp"
#include "components/types.hpp"
#include "modules/meta/base.hpp"

#include "common.hpp"

POLYBAR_NS

/**
 * Add an icon to the vector of icons
 */
void icon_manager::add_icon(surface_t surface, uint64_t id, modules::module_interface* module) {
  std::lock_guard<std::mutex> guard(m_mutex);
  auto d = icon_data{surface, id, module};
  cairo_surface_reference(*(d.surface));
  m_icons.push_back(d);
}

/**
 * Get icon by id and module if exists, otherwise get generic missing icon
 */
surface_t icon_manager::get_icon(uint64_t id) {
  auto icon = find_if(m_icons.begin(), m_icons.end(), [&](const struct icon_data& i) {
    return i.id == id;
  });
  if (icon != m_icons.end()) {
    return icon->surface;
  }

  return m_missing_icon;
}

/**
 * Remove all icons belonging to a module
 */
void icon_manager::clear_icons(modules::module_interface* module) {
  std::lock_guard<std::mutex> guard(m_mutex);

  m_icons.erase(remove_if(m_icons.begin(), m_icons.end(), [&module](const struct icon_data& i) {
    return i.module == module;
  }), m_icons.end());
}
POLYBAR_NS_END

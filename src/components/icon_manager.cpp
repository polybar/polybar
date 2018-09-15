#include "components/icon_manager.hpp"
#include "components/types.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

icon_manager::icon_manager() {}

/**
 * Add an icon to the vector of icons
 */
void icon_manager::add_icon(vector<unsigned char> buf, uint64_t id, modules::module_interface* module) {
  std::lock_guard<std::mutex> guard(m_mutex);
  m_icons.emplace_back(buf, id, module);
}

/**
 * Get icon by id and module if exists, otherwise get generic missing icon
 */
//vector<unsigned char>& icon_manager::get_icon(uint64_t id, modules::module_interface* module) {
// need module to get icon? what if two ids are the same, but diff modules?
vector<unsigned char>& icon_manager::get_icon(uint64_t id) {
  auto icon = find_if(m_icons.begin(), m_icons.end(), [&](const struct icon_data& i) {
    //return i.id == id && i.module == module;
    return i.id == id;
  });
  if (icon != m_icons.end()) {
    return icon->buf;
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

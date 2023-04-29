#include "modules/tray.hpp"

#include "modules/meta/base.inl"
#include "x11/background_manager.hpp"

POLYBAR_NS
namespace modules {
  template class module<tray_module>;

  tray_module::tray_module(const bar_settings& bar_settings, string name_, const config_ini& config_ini)
      : static_module<tray_module>(bar_settings, move(name_), config_ini)
      , m_tray(connection::make(), signal_emitter::make(), m_log, bar_settings, [&] { this->broadcast(); }) {
    m_formatter->add(DEFAULT_FORMAT, TAG_TRAY, {TAG_TRAY});
  }

  string tray_module::get_format() const {
    return DEFAULT_FORMAT;
  }

  void tray_module::start() {
    m_tray.setup(m_conf, name());
    this->static_module<tray_module>::start();
  }

  bool tray_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_TRAY) {
      builder->control(tags::controltag::t);
      extent_val offset_extent = {extent_type::PIXEL, static_cast<float>(m_tray.get_width())};
      builder->offset(offset_extent);
      return true;
    }
    return false;
  }

} // namespace modules
POLYBAR_NS_END

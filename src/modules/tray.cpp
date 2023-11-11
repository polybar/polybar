#include "modules/tray.hpp"

#include "modules/meta/base.inl"
#include "x11/background_manager.hpp"

POLYBAR_NS
namespace modules {
  template class module<tray_module>;

  tray_module::tray_module(const bar_settings& bar_settings, string name_, const config& config)
      : static_module<tray_module>(bar_settings, std::move(name_), config)
      , m_tray(connection::make(), signal_emitter::make(), m_log, bar_settings, [&] { this->broadcast(); }) {
    m_formatter->add(DEFAULT_FORMAT, TAG_TRAY, {TAG_TRAY});

    /* There are a bunch of edge cases with regards to tray visiblity when the
     * <tray> tag is not there (in that case the tray icons should under no
     * circumnstances appear). To avoid this, we simply disallow the situation.
     * The module is basically useless without that tag anyway.
     */
    if (!m_formatter->has(TAG_TRAY, DEFAULT_FORMAT)) {
      throw module_error("The " + std::string(TAG_TRAY) + " tag is required in " + name() + "." + DEFAULT_FORMAT);
    }

    // Otherwise the tray does not see the initial module visibility
    m_tray.change_visibility(visible());
  }

  string tray_module::get_format() const {
    return DEFAULT_FORMAT;
  }

  void tray_module::set_visible(bool value) {
    m_tray.change_visibility(value);
    static_module<tray_module>::set_visible(value);
  }

  void tray_module::start() {
    m_tray.setup(m_conf, name());
    this->static_module<tray_module>::start();
  }

  bool tray_module::build(builder* builder, const string& tag) const {
    // Don't produce any output if the tray is empty so that the module can be hidden
    if (tag == TAG_TRAY && m_tray.get_width() > 0) {
      builder->control(tags::controltag::t);
      extent_val offset_extent = {extent_type::PIXEL, static_cast<float>(m_tray.get_width())};
      builder->offset(offset_extent);
      return true;
    }

    return false;
  }

} // namespace modules
POLYBAR_NS_END

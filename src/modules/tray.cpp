//
// Created by raffael on 02.02.22.
//

#include "modules/tray.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS
namespace modules {
  template class module<tray_module>;

  tray_module::tray_module(const bar_settings& bar_settings, string name_)
      : event_module<tray_module>(bar_settings, move(name_)) {
    m_formatter->add(DEFAULT_FORMAT, "", {});
  }

  string tray_module::get_output() {
    string output{event_module::get_output()};

    m_builder->control(tags::controltag::t);
    m_builder->offset(width);
    return m_builder->flush();
  }

  string tray_module::get_format() const {
    return DEFAULT_FORMAT;
  }

  bool tray_module::has_event() {
    return toUpdate;
  }

  bool tray_module::build(builder* builder, const string& tag) const {
    builder->control(tags::controltag::t);
    builder->offset(width);

    return true;
  }
  bool tray_module::on(const signals::ui_tray::tray_width_change& evt) {
    toUpdate = true;
    width = evt.cast();
    return true;
  }
  bool tray_module::update() {
    return true;
  }

}  // namespace modules
POLYBAR_NS_END

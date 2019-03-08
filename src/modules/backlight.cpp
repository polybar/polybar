#include "modules/backlight.hpp"

#include <cmath>

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/file.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<backlight_module>;

  backlight_module::backlight_module(const bar_settings& bar, string name_)
      : udev_module<backlight_module>(bar, move(name_)), m_card{m_conf.get(name(), "card")} {
    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR, TAG_RAMP});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%%");
    }
    if (m_formatter->has(TAG_BAR)) {
      m_progressbar = load_progressbar(m_bar, m_conf, name(), TAG_BAR);
    }
    if (m_formatter->has(TAG_RAMP)) {
      m_ramp = load_ramp(m_conf, name(), TAG_RAMP);
    }

    auto dev = udev_util::get_device(m_card);
    if (!dev) {
      throw application_error(sstream() << "Specified card: " << m_card << " doesn't exist");
    }

    // Warm up module output before entering the loop
    on_event(udev_event{move(dev)});

    // Add udev watch
    watch("backlight");
  }

  void backlight_module::idle() {
    sleep(75ms);
  }

  bool backlight_module::on_event(udev_event&& event) {
    if (event.dev.get_sysname() != m_card && event.dev.get_action() != "change"s) {
      return false;
    }

    if (event.dev) {
      m_log.trace("%s: %s", name(), event.dev.get_syspath());
    }

    // Documentation of the available attributes
    // https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-class-backlight
    float brightness = strtof(event.dev.get_sysattr("actual_brightness"), nullptr);
    float max_brightness = strtof(event.dev.get_sysattr("max_brightness"), nullptr);

    m_percentage = static_cast<int>(std::round(brightness / max_brightness * 100.0f));

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(m_percentage));
    }

    return true;
  }

  bool backlight_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR) {
      builder->node(m_progressbar->output(m_percentage));
    } else if (tag == TAG_RAMP) {
      builder->node(m_ramp->get_by_percentage(m_percentage));
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }
    return true;
  }
}  // namespace modules

POLYBAR_NS_END

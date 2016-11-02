#include "modules/battery.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace modules {
  void battery_module::setup() {
    // Load configuration values {{{

    m_battery = m_conf.get<string>(name(), "battery", "BAT0");
    m_adapter = m_conf.get<string>(name(), "adapter", "ADP1");
    m_fullat = m_conf.get<int>(name(), "full-at", 100);

    m_path_capacity = string_util::replace(PATH_BATTERY_CAPACITY, "%battery%", m_battery);
    m_path_adapter = string_util::replace(PATH_ADAPTER_STATUS, "%adapter%", m_adapter);

    // }}}
    // Validate paths {{{

    if (!file_util::exists(m_path_capacity))
      throw module_error("The file '" + m_path_capacity + "' does not exist");
    if (!file_util::exists(m_path_adapter))
      throw module_error("The file '" + m_path_adapter + "' does not exist");

    // }}}
    // Load state and capacity level {{{

    m_percentage = current_percentage();
    m_state = current_state();

    // }}}
    // Add formats and elements {{{

    m_formatter->add(FORMAT_CHARGING, TAG_LABEL_CHARGING,
        {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_CHARGING, TAG_LABEL_CHARGING});
    m_formatter->add(FORMAT_DISCHARGING, TAG_LABEL_DISCHARGING,
        {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_DISCHARGING});
    m_formatter->add(
        FORMAT_FULL, TAG_LABEL_FULL, {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_FULL});

    if (m_formatter->has(TAG_ANIMATION_CHARGING, FORMAT_CHARGING))
      m_animation_charging = load_animation(m_conf, name(), TAG_ANIMATION_CHARGING);
    if (m_formatter->has(TAG_BAR_CAPACITY))
      m_bar_capacity = load_progressbar(m_bar, m_conf, name(), TAG_BAR_CAPACITY);
    if (m_formatter->has(TAG_RAMP_CAPACITY))
      m_ramp_capacity = load_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
    if (m_formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING)) {
      m_label_charging = load_optional_label(m_conf, name(), TAG_LABEL_CHARGING, "%percentage%");
    }
    if (m_formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING)) {
      m_label_discharging =
          load_optional_label(m_conf, name(), TAG_LABEL_DISCHARGING, "%percentage%");
    }
    if (m_formatter->has(TAG_LABEL_FULL, FORMAT_FULL)) {
      m_label_full = load_optional_label(m_conf, name(), TAG_LABEL_FULL, "%percentage%");
    }

    // }}}
    // Create inotify watches {{{

    watch(m_path_capacity, IN_ACCESS);
    watch(m_path_adapter, IN_ACCESS);

    // }}}
  }

  void battery_module::start() {
    inotify_module::start();
    m_threads.emplace_back(thread(&battery_module::subthread, this));
  }

  void battery_module::teardown() {
    wakeup();
  }

  bool battery_module::on_event(inotify_event* event) {
    if (event != nullptr) {
      m_log.trace("%s: %s", name(), event->filename);
      m_notified.store(true, std::memory_order_relaxed);
    }

    auto state = current_state();
    int percentage = m_percentage;

    if (state != battery_state::FULL) {
      percentage = current_percentage();
    }

    // Ignore unchanged state
    if (event != nullptr && m_state == state && m_percentage == percentage) {
      return false;
    }

    m_percentage = percentage;
    m_state = state;

    if (m_label_charging) {
      m_label_charging->reset_tokens();
      m_label_charging->replace_token("%percentage%", to_string(m_percentage) + "%");
    }
    if (m_label_discharging) {
      m_label_discharging->reset_tokens();
      m_label_discharging->replace_token("%percentage%", to_string(m_percentage) + "%");
    }
    if (m_label_full) {
      m_label_full->reset_tokens();
      m_label_full->replace_token("%percentage%", to_string(m_percentage) + "%");
    }

    return true;
  }

  string battery_module::get_format() const {
    if (m_state == battery_state::FULL)
      return FORMAT_FULL;
    else if (m_state == battery_state::CHARGING)
      return FORMAT_CHARGING;
    else
      return FORMAT_DISCHARGING;
  }

  bool battery_module::build(builder* builder, string tag) const {
    if (tag == TAG_ANIMATION_CHARGING)
      builder->node(m_animation_charging->get());
    else if (tag == TAG_BAR_CAPACITY) {
      builder->node(m_bar_capacity->output(m_percentage));
    } else if (tag == TAG_RAMP_CAPACITY)
      builder->node(m_ramp_capacity->get_by_percentage(m_percentage));
    else if (tag == TAG_LABEL_CHARGING)
      builder->node(m_label_charging);
    else if (tag == TAG_LABEL_DISCHARGING)
      builder->node(m_label_discharging);
    else if (tag == TAG_LABEL_FULL)
      builder->node(m_label_full);
    else
      return false;
    return true;
  }

  battery_state battery_module::current_state() {
    auto adapter_status = file_util::get_contents(m_path_adapter);

    if (adapter_status.empty()) {
      return battery_state::UNKNOWN;
    } else if (adapter_status[0] == '0') {
      return battery_state::DISCHARGING;
    } else if (adapter_status[0] != '1') {
      return battery_state::UNKNOWN;
    } else if (m_percentage < m_fullat) {
      return battery_state::CHARGING;
    } else {
      return battery_state::FULL;
    }
  }

  /**
   * Get the current capacity level
   */
  int battery_module::current_percentage() {
    auto capacity = file_util::get_contents(m_path_capacity);
    auto value = math_util::cap<int>(std::atof(capacity.c_str()), 0, 100);

    if (value >= m_fullat) {
      return 100;
    } else {
      return value;
    }
  }

  /**
   * Subthread runner that emit update events
   * to refresh <animation-charging> in case it is used.
   *
   * Will also poll for events as fallback for systems that
   * doesn't report inotify events for files on sysfs
   */
  void battery_module::subthread() {
    chrono::duration<double> dur = 1s;

    if (m_animation_charging) {
      dur = chrono::duration<double>(float(m_animation_charging->framerate()) / 1000.0f);
    }

    const int interval = m_conf.get<float>(name(), "poll-interval", 3.0f) / dur.count();

    while (running()) {
      for (int i = 0; running() && i < interval; ++i) {
        if (m_state == battery_state::CHARGING) {
          broadcast();
        }
        sleep(dur);
      }

      if (!running() || m_state == battery_state::CHARGING) {
        continue;
      }

      if (!m_notified.load(std::memory_order_relaxed)) {
        file_util::get_contents(m_path_capacity);
      }
    }

    m_log.trace("%s: End of subthread", name());
  }
}

LEMONBUDDY_NS_END

#include "modules/battery.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Bootstrap module by setting up required components
   */
  void battery_module::setup() {
    // Load configuration values
    auto battery = m_conf.get<string>(name(), "battery", "BAT0");
    auto adapter = m_conf.get<string>(name(), "adapter", "ADP1");

    m_valuepath[battery_value::ADAPTER] = string_util::replace(PATH_ADAPTER_STATUS, "%adapter%", adapter);
    m_valuepath[battery_value::CAPACITY] = string_util::replace(PATH_BATTERY_CAPACITY, "%battery%", battery);
    m_valuepath[battery_value::CAPACITY_MAX] = string_util::replace(PATH_BATTERY_CAPACITY_MAX, "%battery%", battery);
    m_valuepath[battery_value::CAPACITY_PERC] = string_util::replace(PATH_BATTERY_CAPACITY_PERC, "%battery%", battery);
    m_valuepath[battery_value::VOLTAGE] = string_util::replace(PATH_BATTERY_VOLTAGE, "%battery%", battery);
    m_valuepath[battery_value::RATE] = string_util::replace(PATH_BATTERY_RATE, "%battery%", battery);

    for (auto&& file : m_valuepath) {
      if (!file_util::exists(file.second))
        throw module_error("The file '" + file.second + "' does not exist");
    }

    m_fullat = m_conf.get<int>(name(), "full-at", 100);
    m_interval = interval_t{m_conf.get<float>(name(), "poll-interval", 5.0f)};
    m_lastpoll = chrono::system_clock::now();

    // Load state and capacity level
    m_percentage = current_percentage();
    m_state = current_state();

    // Add formats and elements
    m_formatter->add(FORMAT_CHARGING, TAG_LABEL_CHARGING,
        {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_CHARGING, TAG_LABEL_CHARGING});
    m_formatter->add(
        FORMAT_DISCHARGING, TAG_LABEL_DISCHARGING, {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_DISCHARGING});
    m_formatter->add(FORMAT_FULL, TAG_LABEL_FULL, {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_FULL});

    if (m_formatter->has(TAG_ANIMATION_CHARGING, FORMAT_CHARGING))
      m_animation_charging = load_animation(m_conf, name(), TAG_ANIMATION_CHARGING);
    if (m_formatter->has(TAG_BAR_CAPACITY))
      m_bar_capacity = load_progressbar(m_bar, m_conf, name(), TAG_BAR_CAPACITY);
    if (m_formatter->has(TAG_RAMP_CAPACITY))
      m_ramp_capacity = load_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
    if (m_formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING))
      m_label_charging = load_optional_label(m_conf, name(), TAG_LABEL_CHARGING, "%percentage%");
    if (m_formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING))
      m_label_discharging = load_optional_label(m_conf, name(), TAG_LABEL_DISCHARGING, "%percentage%");
    if (m_formatter->has(TAG_LABEL_FULL, FORMAT_FULL))
      m_label_full = load_optional_label(m_conf, name(), TAG_LABEL_FULL, "%percentage%");

    // Create inotify watches
    watch(m_valuepath[battery_value::CAPACITY_PERC], IN_ACCESS);
    watch(m_valuepath[battery_value::ADAPTER], IN_ACCESS);

    // Setup time if token is used
    if (m_label_charging->has_token("%time%") || m_label_discharging->has_token("%time%")) {
      if (!m_bar.locale.empty())
        setlocale(LC_TIME, m_bar.locale.c_str());
      m_timeformat = m_conf.get<string>(name(), "time-format", "%H:%M:%S");
    }
  }

  /**
   * Dispatch the subthread used to update the
   * charging animation when the module is started
   */
  void battery_module::start() {
    inotify_module::start();
    m_threads.emplace_back(thread(&battery_module::subthread, this));
  }

  /**
   * Release wake lock when stopping the module
   */
  void battery_module::teardown() {
    wakeup();
  }

  /**
   * Idle between polling inotify watches for events.
   *
   * If the defined interval has been reached, trigger a manual
   * poll in case the inotify events aren't fired.
   *
   * This fallback is needed because some systems won't
   * report inotify events for files on sysfs.
   */
  void battery_module::idle() {
    if (m_interval.count() > 0) {
      auto now = chrono::system_clock::now();

      if (chrono::duration_cast<decltype(m_interval)>(now - m_lastpoll) > m_interval) {
        m_lastpoll = now;
        m_log.info("%s: Polling values (inotify fallback)", name());
        file_util::get_contents(m_valuepath[battery_value::CAPACITY_PERC]);
      }
    }

    inotify_module::idle();
  }

  /**
   * Update values when tracked files have changed
   */
  bool battery_module::on_event(inotify_event* event) {
    if (event != nullptr) {
      m_log.trace("%s: Inotify event reported for %s", name(), event->filename);
    }

    // Reset timer to avoid unnecessary polling
    m_lastpoll = chrono::system_clock::now();

    auto state = current_state();
    int percentage = m_percentage;

    if (state != battery_state::FULL) {
      percentage = current_percentage();
    }

    if (event != nullptr && state == m_state && percentage == m_percentage && m_unchanged--) {
      return false;
    }

    m_percentage = percentage;
    m_state = state;
    m_unchanged = SKIP_N_UNCHANGED;

    string time_remaining;

    if (m_state == battery_state::CHARGING && m_label_charging) {
      if (!m_timeformat.empty())
        time_remaining = current_time();
      m_label_charging->reset_tokens();
      m_label_charging->replace_token("%percentage%", to_string(m_percentage) + "%");
      m_label_charging->replace_token("%time%", time_remaining);
    } else if (m_state == battery_state::DISCHARGING && m_label_discharging) {
      if (!m_timeformat.empty())
        time_remaining = current_time();
      m_label_discharging->reset_tokens();
      m_label_discharging->replace_token("%percentage%", to_string(m_percentage) + "%");
      m_label_discharging->replace_token("%time%", time_remaining);
    } else if (m_state == battery_state::FULL && m_label_full) {
      m_label_full->reset_tokens();
      m_label_full->replace_token("%percentage%", to_string(m_percentage) + "%");
    }

    return true;
  }

  /**
   * Get the output format based on state
   */
  string battery_module::get_format() const {
    if (m_state == battery_state::FULL)
      return FORMAT_FULL;
    else if (m_state == battery_state::CHARGING)
      return FORMAT_CHARGING;
    else
      return FORMAT_DISCHARGING;
  }

  /**
   * Generate the module output using defined drawtypes
   */
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

  /**
   * Get the current battery state
   */
  battery_state battery_module::current_state() {
    auto adapter_status = file_util::get_contents(m_valuepath[battery_value::ADAPTER]);

    if (adapter_status.empty()) {
      return battery_state::DISCHARGING;
    } else if (adapter_status[0] == '0') {
      return battery_state::DISCHARGING;
    } else if (adapter_status[0] != '1') {
      return battery_state::DISCHARGING;
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
    auto capacity = file_util::get_contents(m_valuepath[battery_value::CAPACITY_PERC]);
    auto value = math_util::cap<int>(std::atof(capacity.c_str()), 0, 100);

    if (value >= m_fullat) {
      return 100;
    } else {
      return value;
    }
  }

  /**
   * Get estimate of remaining time until fully dis-/charged
   */
  string battery_module::current_time() {
    if (m_state == battery_state::FULL) {
      return "";
    }

    int rate{atoi(file_util::get_contents(m_valuepath[battery_value::RATE]).c_str()) / 1000};
    int volt{atoi(file_util::get_contents(m_valuepath[battery_value::VOLTAGE]).c_str()) / 1000};
    int now{atoi(file_util::get_contents(m_valuepath[battery_value::CAPACITY]).c_str()) / 1000};
    int max{atoi(file_util::get_contents(m_valuepath[battery_value::CAPACITY_MAX]).c_str()) / 1000};
    int cap{0};

    if (m_state == battery_state::CHARGING) {
      cap = max - now;
    } else if (m_state == battery_state::DISCHARGING) {
      cap = now;
    }

    struct tm t {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    if (rate && volt && cap) {
      cap = cap * 1000 / volt;
      rate = rate * 1000 / volt;

      if (!rate)
        rate = -1;

      chrono::seconds sec{3600 * cap / rate};

      m_log.trace("%s: sec=%d %d%% cap=%lu rate=%lu volt=%lu", name(), sec.count(), static_cast<int>(m_percentage), cap,
          rate, volt);

      if (sec.count() > 0) {
        t.tm_hour = chrono::duration_cast<chrono::hours>(sec).count();
        sec -= chrono::seconds{3600 * t.tm_hour};
        t.tm_min = chrono::duration_cast<chrono::minutes>(sec).count();
        sec -= chrono::seconds{60 * t.tm_min};
        t.tm_sec = chrono::duration_cast<chrono::seconds>(sec).count();
      }
    }

    char buffer[256]{0};
    strftime(buffer, sizeof(buffer), m_timeformat.c_str(), &t);

    return {buffer};
  }

  /**
   * Subthread runner that emit update events
   * to refresh <animation-charging> in case it is used.
   */
  void battery_module::subthread() {
    chrono::duration<double> dur = 1s;

    if (m_animation_charging) {
      dur = chrono::duration<double>(float(m_animation_charging->framerate()) / 1000.0f);
    }

    while (running()) {
      for (int i = 0; running() && i < dur.count(); ++i) {
        if (m_state == battery_state::CHARGING) {
          broadcast();
        }

        sleep(dur);
      }
    }

    m_log.trace("%s: End of subthread", name());
  }
}

POLYBAR_NS_END

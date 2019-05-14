#include "modules/battery.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/file.hpp"
#include "utils/math.hpp"
#include "utils/string.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<battery_module>;

  template <typename ValueReader>
  typename ValueReader::return_type read(ValueReader& reader) {
    std::lock_guard<ValueReader> guard(reader);
    return reader.read();
  }

  /**
   * Bootstrap module by setting up required components
   */
  battery_module::battery_module(const bar_settings& bar, string name_)
      : inotify_module<battery_module>(bar, move(name_)) {
    // Load configuration values
    m_fullat = math_util::min(m_conf.get(name(), "full-at", m_fullat), 100);
    m_interval = m_conf.get<decltype(m_interval)>(name(), "poll-interval", 5s);
    m_lastpoll = chrono::system_clock::now();

    auto path_adapter = string_util::replace(PATH_ADAPTER, "%adapter%", m_conf.get(name(), "adapter", "ADP1"s)) + "/";
    auto path_battery = string_util::replace(PATH_BATTERY, "%battery%", m_conf.get(name(), "battery", "BAT0"s)) + "/";

    // Make state reader
    if (file_util::exists((m_fstate = path_adapter + "online"))) {
      m_state_reader = make_unique<state_reader>([=] { return file_util::contents(m_fstate).compare(0, 1, "1") == 0; });
    } else if (file_util::exists((m_fstate = path_battery + "status"))) {
      m_state_reader =
          make_unique<state_reader>([=] { return file_util::contents(m_fstate).compare(0, 8, "Charging") == 0; });
    } else {
      throw module_error("No suitable way to get current charge state");
    }

    // Make capacity reader
    if ((m_fcapnow = file_util::pick({path_battery + "charge_now", path_battery + "energy_now"})).empty()) {
      throw module_error("No suitable way to get current capacity value");
    } else if ((m_fcapfull = file_util::pick({path_battery + "charge_full", path_battery + "energy_full"})).empty()) {
      throw module_error("No suitable way to get max capacity value");
    }

    m_capacity_reader = make_unique<capacity_reader>([=] {
      auto cap_now = std::strtoul(file_util::contents(m_fcapnow).c_str(), nullptr, 10);
      auto cap_max = std::strtoul(file_util::contents(m_fcapfull).c_str(), nullptr, 10);
      return math_util::percentage(cap_now, 0UL, cap_max);
    });

    // Make rate reader
    if ((m_fvoltage = file_util::pick({path_battery + "voltage_now"})).empty()) {
      throw module_error("No suitable way to get current voltage value");
    } else if ((m_frate = file_util::pick({path_battery + "current_now", path_battery + "power_now"})).empty()) {
      throw module_error("No suitable way to get current charge rate value");
    }

    m_rate_reader = make_unique<rate_reader>([this] {
      unsigned long rate{std::strtoul(file_util::contents(m_frate).c_str(), nullptr, 10)};
      unsigned long volt{std::strtoul(file_util::contents(m_fvoltage).c_str(), nullptr, 10) / 1000UL};
      unsigned long now{std::strtoul(file_util::contents(m_fcapnow).c_str(), nullptr, 10)};
      unsigned long max{std::strtoul(file_util::contents(m_fcapfull).c_str(), nullptr, 10)};
      unsigned long cap{read(*m_state_reader) ? max - now : now};

      if (rate && volt && cap) {
        auto remaining = (cap / volt);
        auto current_rate = (rate / volt);

        if (remaining && current_rate) {
          return 3600UL * remaining / current_rate;
        }
      }

      return 0UL;
    });

    // Make consumption reader
    m_consumption_reader = make_unique<consumption_reader>([this] {
      float consumption;

      // if the rate we found was the current, calculate power (P = I*V)
      if (string_util::contains(m_frate, "current_now")) {
        unsigned long current{std::strtoul(file_util::contents(m_frate).c_str(), nullptr, 10)};
        unsigned long voltage{std::strtoul(file_util::contents(m_fvoltage).c_str(), nullptr, 10)};

        consumption = ((voltage / 1000.0) * (current /  1000.0)) / 1e6;
      // if it was power, just use as is
      } else {
        unsigned long power{std::strtoul(file_util::contents(m_frate).c_str(), nullptr, 10)};

        consumption = power / 1e6;
      }

      // convert to string with 2 decimmal places
      string rtn(16, '\0'); // 16 should be plenty big. Cant see it needing more than 6/7..
      auto written = std::snprintf(&rtn[0], rtn.size(), "%.2f", consumption);
      rtn.resize(written);

      return rtn;
    });

    // Load state and capacity level
    m_state = current_state();
    m_percentage = current_percentage();

    // Add formats and elements
    m_formatter->add(FORMAT_CHARGING, TAG_LABEL_CHARGING,
        {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_CHARGING, TAG_LABEL_CHARGING});
    m_formatter->add(FORMAT_DISCHARGING, TAG_LABEL_DISCHARGING,
        {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_DISCHARGING, TAG_LABEL_DISCHARGING});
    m_formatter->add(FORMAT_FULL, TAG_LABEL_FULL, {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_FULL});

    if (m_formatter->has(TAG_ANIMATION_CHARGING, FORMAT_CHARGING)) {
      m_animation_charging = load_animation(m_conf, name(), TAG_ANIMATION_CHARGING);
    }
    if (m_formatter->has(TAG_ANIMATION_DISCHARGING, FORMAT_DISCHARGING)) {
      m_animation_discharging = load_animation(m_conf, name(), TAG_ANIMATION_DISCHARGING);
    }
    if (m_formatter->has(TAG_BAR_CAPACITY)) {
      m_bar_capacity = load_progressbar(m_bar, m_conf, name(), TAG_BAR_CAPACITY);
    }
    if (m_formatter->has(TAG_RAMP_CAPACITY)) {
      m_ramp_capacity = load_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
    }
    if (m_formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING)) {
      m_label_charging = load_optional_label(m_conf, name(), TAG_LABEL_CHARGING, "%percentage%%");
    }
    if (m_formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING)) {
      m_label_discharging = load_optional_label(m_conf, name(), TAG_LABEL_DISCHARGING, "%percentage%%");
    }
    if (m_formatter->has(TAG_LABEL_FULL, FORMAT_FULL)) {
      m_label_full = load_optional_label(m_conf, name(), TAG_LABEL_FULL, "%percentage%%");
    }

    // Create inotify watches
    watch(m_fcapnow, IN_ACCESS);
    watch(m_fstate, IN_ACCESS);

    // Setup time if token is used
    if ((m_label_charging && m_label_charging->has_token("%time%")) ||
        (m_label_discharging && m_label_discharging->has_token("%time%"))) {
      if (!m_bar.locale.empty()) {
        setlocale(LC_TIME, m_bar.locale.c_str());
      }
      m_timeformat = m_conf.get(name(), "time-format", "%H:%M:%S"s);
    }
  }

  /**
   * Dispatch the subthread used to update the
   * charging animation when the module is started
   */
  void battery_module::start() {
    this->inotify_module::start();
    m_subthread = thread(&battery_module::subthread, this);
  }

  /**
   * Release wake lock when stopping the module
   */
  void battery_module::teardown() {
    if (m_subthread.joinable()) {
      m_subthread.join();
    }
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
        read(*m_capacity_reader);
      }
    }

    this->inotify_module::idle();
  }

  /**
   * Update values when tracked files have changed
   */
  bool battery_module::on_event(inotify_event* event) {
    auto state = current_state();
    auto percentage = current_percentage();

    // Reset timer to avoid unnecessary polling
    m_lastpoll = chrono::system_clock::now();

    if (event != nullptr) {
      m_log.trace("%s: Inotify event reported for %s", name(), event->filename);

      if (state == m_state && percentage == m_percentage && m_unchanged--) {
        return false;
      }

      m_unchanged = SKIP_N_UNCHANGED;
    }

    m_state = state;
    m_percentage = percentage;

    const auto label = [this] {
      if (m_state == battery_module::state::FULL) {
        return m_label_full;
      } else if (m_state == battery_module::state::DISCHARGING) {
        return m_label_discharging;
      } else {
        return m_label_charging;
      }
    }();

    if (label) {
      label->reset_tokens();
      label->replace_token("%percentage%", to_string(clamp_percentage(m_percentage, m_state)));
      label->replace_token("%percentage_raw%", to_string(m_percentage));
      label->replace_token("%consumption%", current_consumption());

      if (m_state != battery_module::state::FULL && !m_timeformat.empty()) {
        label->replace_token("%time%", current_time());
      }
    }

    return true;
  }

  /**
   * Get the output format based on state
   */
  string battery_module::get_format() const {
    if (m_state == battery_module::state::CHARGING) {
      return FORMAT_CHARGING;
    } else if (m_state == battery_module::state::DISCHARGING) {
      return FORMAT_DISCHARGING;
    } else {
      return FORMAT_FULL;
    }
  }

  /**
   * Generate module output using defined drawtypes
   */
  bool battery_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_ANIMATION_CHARGING) {
      builder->node(m_animation_charging->get());
    } else if (tag == TAG_ANIMATION_DISCHARGING) {
      builder->node(m_animation_discharging->get());
    } else if (tag == TAG_BAR_CAPACITY) {
      builder->node(m_bar_capacity->output(clamp_percentage(m_percentage, m_state)));
    } else if (tag == TAG_RAMP_CAPACITY) {
      builder->node(m_ramp_capacity->get_by_percentage(clamp_percentage(m_percentage, m_state)));
    } else if (tag == TAG_LABEL_CHARGING) {
      builder->node(m_label_charging);
    } else if (tag == TAG_LABEL_DISCHARGING) {
      builder->node(m_label_discharging);
    } else if (tag == TAG_LABEL_FULL) {
      builder->node(m_label_full);
    } else {
      return false;
    }

    return true;
  }

  /**
   * Get the current battery state
   */
  battery_module::state battery_module::current_state() {
    if (!read(*m_state_reader)) {
      return battery_module::state::DISCHARGING;
    } else if (read(*m_capacity_reader) < m_fullat) {
      return battery_module::state::CHARGING;
    } else {
      return battery_module::state::FULL;
    }
  }

  /**
   * Get the current capacity level
   */
  int battery_module::current_percentage() {
    return read(*m_capacity_reader);
  }

  int battery_module::clamp_percentage(int percentage, state state) const {
    if (state == battery_module::state::FULL && percentage >= m_fullat) {
      return 100;
    }
    return percentage;
  }

  /**
  * Get the current power consumption
  */
  string battery_module::current_consumption() {
    return read(*m_consumption_reader);
  }

  /**
   * Get estimate of remaining time until fully dis-/charged
   */
  string battery_module::current_time() {
    struct tm t {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr
    };

    chrono::seconds sec{read(*m_rate_reader)};
    if (sec.count() > 0) {
      t.tm_hour = chrono::duration_cast<chrono::hours>(sec).count();
      sec -= chrono::seconds{3600 * t.tm_hour};
      t.tm_min = chrono::duration_cast<chrono::minutes>(sec).count();
      sec -= chrono::seconds{60 * t.tm_min};
      t.tm_sec = chrono::duration_cast<chrono::seconds>(sec).count();
    }

    char buffer[256]{0};
    strftime(buffer, sizeof(buffer), m_timeformat.c_str(), &t);
    return {buffer};
  }

  /**
   * Subthread runner that emits update events to refresh <animation-charging>
   * or <animation-discharging> in case they are used. Note, that it is ok to
   * use a single thread, because the two animations are never shown at the
   * same time.
   */
  void battery_module::subthread() {
    chrono::duration<double> dur{0.0};

    if (battery_module::state::CHARGING == m_state && m_animation_charging) {
      dur += chrono::milliseconds{m_animation_charging->framerate()};
    } else if (battery_module::state::DISCHARGING == m_state && m_animation_discharging) {
      dur += chrono::milliseconds{m_animation_discharging->framerate()};
    } else {
      dur += 1s;
    }

    while (running()) {
      for (int i = 0; running() && i < dur.count(); ++i) {
        if (m_state == battery_module::state::CHARGING ||
            m_state == battery_module::state::DISCHARGING) {
          broadcast();
        }
        sleep(dur);
      }
    }

    m_log.trace("%s: End of subthread", name());
  }
}

POLYBAR_NS_END

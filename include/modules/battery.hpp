#pragma once

#include "config.hpp"
#include "drawtypes/animation.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"
#include "utils/file.hpp"
#include "utils/inotify.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace modules {
  class battery_module : public inotify_module<battery_module> {
   public:
    using inotify_module::inotify_module;

    void setup() {
      // Load configuration values {{{

      m_battery = m_conf.get<string>(name(), "battery", "BAT0");
      m_adapter = m_conf.get<string>(name(), "adapter", "ADP1");
      m_fullat = m_conf.get<int>(name(), "full-at", 100);

      m_path_capacity = string_util::replace(PATH_BATTERY_CAPACITY, "%battery%", m_battery);
      m_path_adapter = string_util::replace(PATH_ADAPTER_STATUS, "%adapter%", m_adapter);

      m_state = STATE_UNKNOWN;
      m_percentage = 0;

      // }}}
      // Validate paths {{{

      if (!file_util::exists(m_path_capacity))
        throw module_error("battery_module: The file '" + m_path_capacity + "' does not exist");
      if (!file_util::exists(m_path_adapter))
        throw module_error("battery_module: The file '" + m_path_adapter + "' does not exist");

      // }}}
      // Add formats and elements {{{

      m_formatter->add(FORMAT_CHARGING, TAG_LABEL_CHARGING,
          {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_ANIMATION_CHARGING, TAG_LABEL_CHARGING});
      m_formatter->add(FORMAT_DISCHARGING, TAG_LABEL_DISCHARGING,
          {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_DISCHARGING});
      m_formatter->add(
          FORMAT_FULL, TAG_LABEL_FULL, {TAG_BAR_CAPACITY, TAG_RAMP_CAPACITY, TAG_LABEL_FULL});

      if (m_formatter->has(TAG_ANIMATION_CHARGING, FORMAT_CHARGING))
        m_animation_charging = get_config_animation(m_conf, name(), TAG_ANIMATION_CHARGING);
      if (m_formatter->has(TAG_BAR_CAPACITY))
        m_bar_capacity = get_config_bar(m_bar, m_conf, name(), TAG_BAR_CAPACITY);
      if (m_formatter->has(TAG_RAMP_CAPACITY))
        m_ramp_capacity = get_config_ramp(m_conf, name(), TAG_RAMP_CAPACITY);
      if (m_formatter->has(TAG_LABEL_CHARGING, FORMAT_CHARGING)) {
        m_label_charging =
            get_optional_config_label(m_conf, name(), TAG_LABEL_CHARGING, "%percentage%");
        m_label_charging_tokenized = m_label_charging->clone();
      }
      if (m_formatter->has(TAG_LABEL_DISCHARGING, FORMAT_DISCHARGING)) {
        m_label_discharging =
            get_optional_config_label(m_conf, name(), TAG_LABEL_DISCHARGING, "%percentage%");
        m_label_discharging_tokenized = m_label_discharging->clone();
      }
      if (m_formatter->has(TAG_LABEL_FULL, FORMAT_FULL)) {
        m_label_full = get_optional_config_label(m_conf, name(), TAG_LABEL_FULL, "%percentage%");
        m_label_full_tokenized = m_label_full->clone();
      }

      // }}}
      // Create inotify watches {{{

      watch(m_path_capacity, IN_ACCESS);
      watch(m_path_adapter, IN_ACCESS);

      // }}}
    }

    void start() {
      m_threads.emplace_back(thread(&battery_module::subthread_routine, this));
      inotify_module::start();
    }

    bool on_event(inotify_event* event) {
      if (event != nullptr)
        m_log.trace("%s: %s", name(), event->filename);

      auto status = file_util::get_contents(m_path_adapter);
      if (status.empty()) {
        m_log.err("%s: Failed to read '%s'", name(), m_path_adapter);
        return false;
      }

      auto capacity = file_util::get_contents(m_path_capacity);
      if (capacity.empty()) {
        m_log.err("%s: Failed to read '%s'", name(), m_path_capacity);
        return false;
      }

      int percentage = math_util::cap<float>(std::atof(capacity.c_str()), 0, 100) + 0.5;
      int state = STATE_UNKNOWN;

      switch (status[0]) {
        case '0':
          state = STATE_DISCHARGING;
          break;
        case '1':
          state = STATE_CHARGING;
          break;
      }

      if (state == STATE_CHARGING) {
        if (percentage >= m_fullat)
          percentage = 100;
        if (percentage == 100)
          state = STATE_FULL;
      }

      // check for nullptr since we don't want to ignore the update for the warmup run
      if (event != nullptr && m_state == state && m_percentage == percentage) {
        m_log.trace("%s: Ignore update since values are unchanged", name());
        return false;
      }

      if (m_label_charging_tokenized) {
        m_label_charging_tokenized->m_text = m_label_charging->m_text;
        m_label_charging_tokenized->replace_token("%percentage%", to_string(percentage) + "%");
      }
      if (m_label_discharging_tokenized) {
        m_label_discharging_tokenized->m_text = m_label_discharging->m_text;
        m_label_discharging_tokenized->replace_token("%percentage%", to_string(percentage) + "%");
      }
      if (m_label_full_tokenized) {
        m_label_full_tokenized->m_text = m_label_full->m_text;
        m_label_full_tokenized->replace_token("%percentage%", to_string(percentage) + "%");
      }

      m_state = state;
      m_percentage = percentage;

      return true;
    }

    string get_format() {
      if (m_state == STATE_FULL)
        return FORMAT_FULL;
      else if (m_state == STATE_CHARGING)
        return FORMAT_CHARGING;
      else
        return FORMAT_DISCHARGING;
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_ANIMATION_CHARGING)
        builder->node(m_animation_charging->get());
      else if (tag == TAG_BAR_CAPACITY) {
        builder->node(m_bar_capacity->output(m_percentage));
      } else if (tag == TAG_RAMP_CAPACITY)
        builder->node(m_ramp_capacity->get_by_percentage(m_percentage));
      else if (tag == TAG_LABEL_CHARGING)
        builder->node(m_label_charging_tokenized);
      else if (tag == TAG_LABEL_DISCHARGING)
        builder->node(m_label_discharging_tokenized);
      else if (tag == TAG_LABEL_FULL)
        builder->node(m_label_full_tokenized);
      else
        return false;
      return true;
    }

   protected:
    void subthread_routine() {
      this_thread::yield();

      chrono::duration<double> dur = 1s;

      if (m_animation_charging)
        dur = chrono::duration<double>(float(m_animation_charging->framerate()) / 1000.0f);

      int i = 0;
      const int poll_seconds = m_conf.get<float>(name(), "poll-interval", 3.0f) / dur.count();

      while (enabled()) {
        // TODO(jaagr): Keep track of when the values were last read to determine
        // if we need to trigger the event manually or not.
        if (poll_seconds > 0 && (++i % poll_seconds) == 0) {
          // Trigger an inotify event in case the underlying filesystem doesn't
          m_log.trace("%s: Poll battery capacity", name());
          file_util::get_contents(m_path_capacity);
          i = 0;
        }

        if (m_state == STATE_CHARGING)
          broadcast();

        sleep(dur);
      }

      m_log.trace("%s: Reached end of battery subthread", name());
    }

   private:
    static const int STATE_UNKNOWN = 1;
    static const int STATE_CHARGING = 2;
    static const int STATE_DISCHARGING = 3;
    static const int STATE_FULL = 4;

    static constexpr auto FORMAT_CHARGING = "format-charging";
    static constexpr auto FORMAT_DISCHARGING = "format-discharging";
    static constexpr auto FORMAT_FULL = "format-full";

    static constexpr auto TAG_ANIMATION_CHARGING = "<animation-charging>";
    static constexpr auto TAG_BAR_CAPACITY = "<bar-capacity>";
    static constexpr auto TAG_RAMP_CAPACITY = "<ramp-capacity>";
    static constexpr auto TAG_LABEL_CHARGING = "<label-charging>";
    static constexpr auto TAG_LABEL_DISCHARGING = "<label-discharging>";
    static constexpr auto TAG_LABEL_FULL = "<label-full>";

    animation_t m_animation_charging;
    ramp_t m_ramp_capacity;
    progressbar_t m_bar_capacity;
    label_t m_label_charging;
    label_t m_label_charging_tokenized;
    label_t m_label_discharging;
    label_t m_label_discharging_tokenized;
    label_t m_label_full;
    label_t m_label_full_tokenized;

    string m_battery;
    string m_adapter;
    string m_path_capacity;
    string m_path_adapter;

    int m_state;
    int m_percentage;
    int m_fullat;
  };
}

LEMONBUDDY_NS_END

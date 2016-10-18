#pragma once

#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class backlight_module : public inotify_module<backlight_module> {
   public:
    using inotify_module::inotify_module;

    void setup() {
      // Load configuration values
      auto card = m_conf.get<string>(name(), "card");

      // Add formats and elements
      m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR, TAG_RAMP});

      if (m_formatter->has(TAG_LABEL))
        m_label = get_optional_config_label(m_conf, name(), TAG_LABEL, "%percentage%");
      if (m_formatter->has(TAG_BAR))
        m_progressbar = get_config_bar(m_bar, m_conf, name(), TAG_BAR);
      if (m_formatter->has(TAG_RAMP))
        m_ramp = get_config_ramp(m_conf, name(), TAG_RAMP);

      // Build path to the file where the current/maximum brightness value is located
      m_path_val = string_util::replace(PATH_BACKLIGHT_VAL, "%card%", card);
      m_path_max = string_util::replace(PATH_BACKLIGHT_MAX, "%card%", card);

      if (!file_util::exists(m_path_val))
        throw module_error("backlight_module: The file '" + m_path_val + "' does not exist");
      if (!file_util::exists(m_path_max))
        throw module_error("backlight_module: The file '" + m_path_max + "' does not exist");

      // Add inotify watch
      watch(string_util::replace(PATH_BACKLIGHT_VAL, "%card%", card));
    }

    bool on_event(inotify_event* event) {
      if (event != nullptr)
        m_log.trace("%s: %s", name(), event->filename);

      auto val = file_util::get_contents(m_path_val);
      m_val = std::stoull(val.c_str(), 0, 10);

      auto max = file_util::get_contents(m_path_max);
      m_max = std::stoull(max.c_str(), 0, 10);

      m_percentage = static_cast<int>(float(m_val) / float(m_max) * 100.0f + 0.5f);

      if (m_label) {
        m_label->reset_tokens();
        m_label->replace_token("%percentage%", to_string(m_percentage) + "%");
      }

      return true;
    }

    bool build(builder* builder, string tag) const {
      if (tag == TAG_BAR)
        builder->node(m_progressbar->output(m_percentage));
      else if (tag == TAG_RAMP)
        builder->node(m_ramp->get_by_percentage(m_percentage));
      else if (tag == TAG_LABEL)
        builder->node(m_label);
      else
        return false;
      return true;
    }

    void idle() {
      this_thread::sleep_for(75ms);
    }

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    string m_path_val;
    string m_path_max;
    float m_val = 0;
    float m_max = 0;

    int m_percentage;
  };
}

LEMONBUDDY_NS_END

#pragma once

#include "config.hpp"
#include "components/config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  struct brightness_handle {
    void filepath(string path) {
      if (!file_util::exists(path))
        throw module_error("The file '" + path + "' does not exist");
      m_path = path;
    }

    float read() const {
      return std::strtof(file_util::get_contents(m_path).c_str(), 0);
    }

   private:
    string m_path;
  };

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
      m_val.filepath(string_util::replace(PATH_BACKLIGHT_VAL, "%card%", card));
      m_max.filepath(string_util::replace(PATH_BACKLIGHT_MAX, "%card%", card));

      // Add inotify watch
      watch(string_util::replace(PATH_BACKLIGHT_VAL, "%card%", card));
    }

    void idle() {
      sleep(75ms);
    }

    bool on_event(inotify_event* event) {
      if (event != nullptr)
        m_log.trace("%s: %s", name(), event->filename);

      m_percentage = static_cast<int>(m_val.read() / m_max.read() * 100.0f + 0.5f);

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

   private:
    static constexpr auto TAG_LABEL = "<label>";
    static constexpr auto TAG_BAR = "<bar>";
    static constexpr auto TAG_RAMP = "<ramp>";

    ramp_t m_ramp;
    label_t m_label;
    progressbar_t m_progressbar;

    brightness_handle m_val;
    brightness_handle m_max;

    int m_percentage = 0;
  };
}

LEMONBUDDY_NS_END

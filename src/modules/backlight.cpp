#include "modules/backlight.hpp"

#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "utils/file.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace modules {
  template class module<backlight_module>;

  void backlight_module::brightness_handle::filepath(const string& path) {
    if (!file_util::exists(path)) {
      throw module_error("The file '" + path + "' does not exist");
    }
    m_path = path;
  }

  float backlight_module::brightness_handle::read() const {
    return std::strtof(file_util::contents(m_path).c_str(), nullptr);
  }

  backlight_module::backlight_module(const bar_settings& bar, string name_)
      : inotify_module<backlight_module>(bar, move(name_)) {
    auto card = m_conf.get(name(), "card");

    // Get flag to check if we should add scroll handlers for changing value
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);

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

    // Build path to the sysfs folder the current/maximum brightness values are located
    m_path_backlight = string_util::replace(PATH_BACKLIGHT, "%card%", card);

    /*
     * amdgpu drivers set the actual_brightness in a different scale than [0, max_brightness]
     * The only sensible way is to use the 'brightness' file instead
     * Ref: https://github.com/Alexays/Waybar/issues/335
     */
    std::string brightness_type = ((card.substr(0, 9) == "amdgpu_bl") ? "brightness" : "actual_brightness");
    auto path_backlight_val = m_path_backlight + "/" + brightness_type;

    m_val.filepath(path_backlight_val);
    m_max.filepath(m_path_backlight + "/max_brightness");

    // Add inotify watch
    watch(path_backlight_val);
  }

  void backlight_module::idle() {
    sleep(75ms);
  }

  bool backlight_module::on_event(inotify_event* event) {
    if (event != nullptr) {
      m_log.trace("%s: %s", name(), event->filename);
    }

    m_max_brightness = m_max.read();
    m_percentage = static_cast<int>(m_val.read() / m_max_brightness * 100.0f + 0.5f);

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(m_percentage));
    }

    return true;
  }

  string backlight_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapped
    // with the cmd handlers
    string output{module::get_output()};

    if (m_scroll) {
      m_builder->cmd(mousebtn::SCROLL_UP, EVENT_SCROLLUP);
      m_builder->cmd(mousebtn::SCROLL_DOWN, EVENT_SCROLLDOWN);
    }

    m_builder->append(std::move(output));

    m_builder->cmd_close();
    m_builder->cmd_close();

    return m_builder->flush();
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

  bool backlight_module::input(string&& cmd) {
    double value_mod{0.0};

    if (cmd == EVENT_SCROLLUP) {
      value_mod = 5.0;
    } else if (cmd == EVENT_SCROLLDOWN) {
      value_mod = -5.0;
    } else {
      return false;
    }

    m_log.info("%s: Changing value by %f%", name(), value_mod);

    try {
      int rounded = math_util::cap<double>(m_percentage + value_mod, 0.0, 100.0) + 0.5;
      int value = math_util::percentage_to_value<int>(rounded, m_max_brightness);
      file_util::write_contents(m_path_backlight + "/brightness", to_string(value));
    } catch (const exception& err) {
      m_log.err(
          "%s: Unable to change backlight value. Your system may require additional "
          "configuration. Please read the module documentation.\n(reason: %s)",
          name(), err.what());
    }

    return true;
  }
}  // namespace modules

POLYBAR_NS_END

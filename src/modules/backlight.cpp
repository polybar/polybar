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
    m_router->register_action(EVENT_DEC, [this]() { action_dec(); });
    m_router->register_action(EVENT_INC, [this]() { action_inc(); });
    m_router->register_action(EVENT_TOGGLE, [this]() { action_toggle(); });

    auto card = m_conf.get(name(), "card");

    // Get flag to check if we should add scroll handlers for changing value
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);

    m_scroll_interval = m_conf.get(name(), "scroll-interval", m_scroll_interval);

    // Clicking allows toggling between MAX and MIN brightness. Enables us to have
    // nice small steps while also being able to increase/decrease brighthess quickly.
    m_click_toggle = m_conf.get(name(), "click-toggle", false);

    // Added missing reverse-scroll
    m_reverse_scroll = m_conf.get(name(), "reverse-scroll", false);

    // Pseudo-logarithmic steps. Human perception of light recognises changes in
    // lower intensity much more than in higher intensity. Meaning that going from
    // 90% to 95% seems to be much smaller change than going from 1% to 6%...
    // For now I chose to go the easy way and set step values as this:
    //  <6%  = step 1%
    //  <20% = step 3%
    // >=21% = step <scroll-interval>% (defaults to 5)
    m_log_scroll = m_conf.get(name(), "logarithmic-scroll", false);

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
    bool card_is_amdgpu = (card.substr(0, 9) == "amdgpu_bl");
    m_use_actual_brightness = m_conf.get(name(), "use-actual-brightness", !card_is_amdgpu);

    std::string brightness_type = (m_use_actual_brightness ? "actual_brightness" : "brightness");
    auto path_backlight_val = m_path_backlight + "/" + brightness_type;

    m_val.filepath(path_backlight_val);
    m_max.filepath(m_path_backlight + "/max_brightness");

    // Add inotify watch
    watch(path_backlight_val);
  }

  void backlight_module::idle() {
    sleep(75ms);
  }

  bool backlight_module::on_event(const inotify_event& event) {
    if (event.is_valid) {
      m_log.trace("%s: %s", name(), event.filename);
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

    m_builder->action(mousebtn::LEFT, *this, EVENT_TOGGLE, "");

    // reverse-scroll is handled in change_value() so I don't have to duplicate
    // these lines.
    if (m_scroll) {
      m_builder->action(mousebtn::SCROLL_UP, *this, EVENT_INC, "");
      m_builder->action(mousebtn::SCROLL_DOWN, *this, EVENT_DEC, "");
    }

    m_builder->node(output);

    m_builder->action_close();
    m_builder->action_close();

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

  // Get inc/dec step so I don't have to repeat the same code or rewrite logic of
  // the change_value().
  inline int backlight_module::get_step() {
    int step = m_scroll_interval;

    // No logarithmic-scroll = use scroll-interval as is
    if (m_log_scroll == false)
      return step;

    if (m_percentage <= 5) {
      step = 1;
    } else if (m_percentage <= 20) {
      step = 3;
    }

    return step;
  }

  // Clicking on backlight results in setting to MIN when not MIN. If already MIN
  // is set, set to MAX.
  void backlight_module::action_toggle() {
    if (m_click_toggle == false) {
      return;
    }

    set_value((m_percentage > 1 ? 1 : m_max_brightness));
  }

  void backlight_module::action_inc() {
    change_value(get_step());
  }

  void backlight_module::action_dec() {
    change_value(-get_step());
  }

  // Relative change of current backlight percentage
  void backlight_module::change_value(int value_mod) {
    // Handle "reverse-scroll"
    if (m_reverse_scroll) {
      value_mod *= -1;
    }

    int rounded = math_util::cap<double>(m_percentage + value_mod, 0.0, 100.0) + 0.5;
    int value = math_util::percentage_to_value<int>(rounded, m_max_brightness);

    // Don't disable backlight, just set the minimum. Maybe add some config to set
    // boundaries or atleast (dis)allow value of 0?
    if (value == 0) {
      value = 1;
    }

    m_log.info("%s: Backlight  [CURR=%d%%  STEP=%d  NEXT=%d%%=>%d", name(), m_percentage, value_mod, rounded, value);
    set_value(value);
  }

  // Setting absolute value by writing in to brightness file
  void backlight_module::set_value(int new_value) {
    try {
      m_log.info("%s: Setting brighness value to %d", name(), new_value);
      file_util::write_contents(m_path_backlight + "/brightness", to_string(new_value));
    } catch (const exception& err) {
      m_log.err(
          "%s: Unable to change backlight value. Your system may require additional "
          "configuration. Please read the module documentation.\n(reason: %s)",
          name(), err.what());
    }
  }

} // namespace modules

POLYBAR_NS_END

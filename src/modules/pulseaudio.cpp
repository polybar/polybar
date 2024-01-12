#include "modules/pulseaudio.hpp"

#include "adapters/pulseaudio.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "settings.hpp"
#include "utils/math.hpp"

POLYBAR_NS

namespace modules {
  template class module<pulseaudio_module>;

  pulseaudio_module::pulseaudio_module(const bar_settings& bar, string name_, const config& config)
      : event_module<pulseaudio_module>(bar, move(name_), config) {
    if (m_handle_events) {
      m_router->register_action(EVENT_DEC, [this]() { action_dec(); });
      m_router->register_action(EVENT_INC, [this]() { action_inc(); });
      m_router->register_action(EVENT_TOGGLE, [this]() { action_toggle(); });
    }

    // Load configuration values
    m_interval = m_conf.get(name(), "interval", m_interval);
    m_unmute_on_scroll = m_conf.get(name(), "unmute-on-scroll", m_unmute_on_scroll);

    auto sink_name = m_conf.get(name(), "sink", ""s);
    bool m_max_volume = m_conf.get(name(), "use-ui-max", true);
    m_reverse_scroll = m_conf.get(name(), "reverse-scroll", false);

    try {
      m_pulseaudio = std::make_unique<pulseaudio>(m_log, move(sink_name), m_max_volume);
    } catch (const pulseaudio_error& err) {
      throw module_error(err.what());
    }

    // Add formats and elements
    m_formatter->add(FORMAT_VOLUME, TAG_LABEL_VOLUME, {TAG_RAMP_VOLUME, TAG_LABEL_VOLUME, TAG_BAR_VOLUME});
    m_formatter->add(FORMAT_MUTED, TAG_LABEL_MUTED, {TAG_RAMP_VOLUME, TAG_LABEL_MUTED, TAG_BAR_VOLUME});

    if (m_formatter->has(TAG_BAR_VOLUME)) {
      m_bar_volume = load_progressbar(m_bar, m_conf, name(), TAG_BAR_VOLUME);
    }
    if (m_formatter->has(TAG_LABEL_VOLUME, FORMAT_VOLUME)) {
      m_label_volume = load_optional_label(m_conf, name(), TAG_LABEL_VOLUME, "%percentage%%");
    }
    if (m_formatter->has(TAG_LABEL_MUTED, FORMAT_MUTED)) {
      m_label_muted = load_optional_label(m_conf, name(), TAG_LABEL_MUTED, "%percentage%%");
    }
    if (m_formatter->has(TAG_RAMP_VOLUME)) {
      m_ramp_volume = load_ramp(m_conf, name(), TAG_RAMP_VOLUME);
    }
  }

  void pulseaudio_module::teardown() {
    m_pulseaudio.reset();
  }

  bool pulseaudio_module::has_event() {
    // Poll for mixer and control events
    try {
      if (m_pulseaudio->wait()) {
        return true;
      }
    } catch (const pulseaudio_error& e) {
      m_log.err("%s: %s", name(), e.what());
    }
    return false;
  }

  bool pulseaudio_module::update() {
    // Consume pending events
    m_pulseaudio->process_events();

    // Get volume and mute state
    m_volume = 100;
    m_decibels = PA_DECIBEL_MININFTY;
    m_muted = false;

    try {
      if (m_pulseaudio) {
        m_volume = m_volume * m_pulseaudio->get_volume() / 100.0f;
        m_decibels = m_pulseaudio->get_decibels();
        m_muted = m_muted || m_pulseaudio->is_muted();
      }
    } catch (const pulseaudio_error& err) {
      m_log.err("%s: Failed to query pulseaudio sink (%s)", name(), err.what());
    }

    // Replace label tokens
    if (m_label_volume) {
      m_label_volume->reset_tokens();
      m_label_volume->replace_token("%percentage%", to_string(m_volume));
      m_label_volume->replace_token("%decibels%", string_util::floating_point(m_decibels, 2, true));
    }

    if (m_label_muted) {
      m_label_muted->reset_tokens();
      m_label_muted->replace_token("%percentage%", to_string(m_volume));
      m_label_muted->replace_token("%decibels%", string_util::floating_point(m_decibels, 2, true));
    }

    return true;
  }

  string pulseaudio_module::get_format() const {
    return m_muted ? FORMAT_MUTED : FORMAT_VOLUME;
  }

  string pulseaudio_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    if (m_handle_events) {
      auto click_middle = m_conf.get(name(), "click-middle", ""s);
      auto click_right = m_conf.get(name(), "click-right", ""s);

      if (!click_middle.empty()) {
        m_builder->action(mousebtn::MIDDLE, click_middle);
      }

      if (!click_right.empty()) {
        m_builder->action(mousebtn::RIGHT, click_right);
      }

      m_builder->action(mousebtn::LEFT, *this, EVENT_TOGGLE, "");
      if (!m_reverse_scroll) {
        m_builder->action(mousebtn::SCROLL_UP, *this, EVENT_INC, "");
        m_builder->action(mousebtn::SCROLL_DOWN, *this, EVENT_DEC, "");
      } else {
        m_builder->action(mousebtn::SCROLL_UP, *this, EVENT_DEC, "");
        m_builder->action(mousebtn::SCROLL_DOWN, *this, EVENT_INC, "");
      }
    }

    m_builder->node(output);

    return m_builder->flush();
  }

  bool pulseaudio_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR_VOLUME) {
      builder->node(m_bar_volume->output(m_volume));
    } else if (tag == TAG_RAMP_VOLUME) {
      builder->node(m_ramp_volume->get_by_percentage(m_volume));
    } else if (tag == TAG_LABEL_VOLUME) {
      builder->node(m_label_volume);
    } else if (tag == TAG_LABEL_MUTED) {
      builder->node(m_label_muted);
    } else {
      return false;
    }
    return true;
  }

  void pulseaudio_module::action_inc() {
    if (m_unmute_on_scroll) {
      m_pulseaudio->set_mute(false);
    }
    m_pulseaudio->inc_volume(m_interval);
  }

  void pulseaudio_module::action_dec() {
    if (m_unmute_on_scroll) {
      m_pulseaudio->set_mute(false);
    }
    m_pulseaudio->inc_volume(-m_interval);
  }

  void pulseaudio_module::action_toggle() {
    m_pulseaudio->toggle_mute();
  }
} // namespace modules

POLYBAR_NS_END

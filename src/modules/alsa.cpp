#include "modules/alsa.hpp"

#include "adapters/alsa/control.hpp"
#include "adapters/alsa/generic.hpp"
#include "adapters/alsa/mixer.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta/base.inl"
#include "settings.hpp"
#include "utils/math.hpp"

POLYBAR_NS

using namespace alsa;

namespace modules {
  template class module<alsa_module>;

  alsa_module::alsa_module(const bar_settings& bar, string name_, const config& config)
      : event_module<alsa_module>(bar, move(name_), config) {
    if (m_handle_events) {
      m_router->register_action(EVENT_DEC, [this]() { action_dec(); });
      m_router->register_action(EVENT_INC, [this]() { action_inc(); });
      m_router->register_action(EVENT_TOGGLE, [this]() { action_toggle(); });
    }

    // Load configuration values
    m_mapped = m_conf.get(name(), "mapped", m_mapped);
    m_interval = m_conf.get(name(), "interval", m_interval);
    m_unmute_on_scroll = m_conf.get(name(), "unmute-on-scroll", m_unmute_on_scroll);

    auto master_mixer_name = m_conf.get(name(), "master-mixer", "Master"s);
    auto speaker_mixer_name = m_conf.get(name(), "speaker-mixer", ""s);
    auto headphone_mixer_name = m_conf.get(name(), "headphone-mixer", ""s);

    // m_soundcard_name: Master Soundcard Name
    // s_soundcard_name: Speaker Soundcard Name
    // h_soundcard_name: Headphone Soundcard Name
    auto m_soundcard_name = m_conf.get(name(), "master-soundcard", "default"s);
    auto s_soundcard_name = m_conf.get(name(), "speaker-soundcard", "default"s);
    auto h_soundcard_name = m_conf.get(name(), "headphone-soundcard", "default"s);

    if (!headphone_mixer_name.empty()) {
      m_headphoneid = m_conf.get<decltype(m_headphoneid)>(name(), "headphone-id");
    }

    if (string_util::compare(speaker_mixer_name, "master")) {
      throw module_error("Master mixer is already defined");
    }
    if (string_util::compare(headphone_mixer_name, "master")) {
      throw module_error("Master mixer is already defined");
    }

    // Setup mixers
    try {
      if (!master_mixer_name.empty()) {
        m_mixer[mixer::MASTER].reset(new mixer_t::element_type{move(master_mixer_name), move(m_soundcard_name)});
      }
      if (!speaker_mixer_name.empty()) {
        m_mixer[mixer::SPEAKER].reset(new mixer_t::element_type{move(speaker_mixer_name), move(s_soundcard_name)});
      }
      if (!headphone_mixer_name.empty()) {
        m_mixer[mixer::HEADPHONE].reset(new mixer_t::element_type{move(headphone_mixer_name), move(h_soundcard_name)});
      }
      if (m_mixer[mixer::HEADPHONE]) {
        m_ctrl[control::HEADPHONE].reset(new control_t::element_type{m_headphoneid});
      }
      if (m_mixer.empty()) {
        throw module_error("No configured mixers");
      }
    } catch (const mixer_error& err) {
      throw module_error(err.what());
    } catch (const control_error& err) {
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
      m_ramp_headphones = load_ramp(m_conf, name(), TAG_RAMP_HEADPHONES, false);
    }
  }

  void alsa_module::teardown() {
    m_mixer.clear();
    m_ctrl.clear();
    snd_config_update_free_global();
  }

  bool alsa_module::has_event() {
    // Poll for mixer and control events
    try {
      if (m_mixer[mixer::MASTER] && m_mixer[mixer::MASTER]->wait(25)) {
        return true;
      }
      if (m_mixer[mixer::SPEAKER] && m_mixer[mixer::SPEAKER]->wait(25)) {
        return true;
      }
      if (m_mixer[mixer::HEADPHONE] && m_mixer[mixer::HEADPHONE]->wait(25)) {
        return true;
      }
      if (m_ctrl[control::HEADPHONE] && m_ctrl[control::HEADPHONE]->wait(25)) {
        return true;
      }
    } catch (const alsa_exception& e) {
      m_log.err("%s: %s", name(), e.what());
    }

    return false;
  }

  bool alsa_module::update() {
    // Consume pending events
    if (m_mixer[mixer::MASTER]) {
      m_mixer[mixer::MASTER]->process_events();
    }
    if (m_mixer[mixer::SPEAKER]) {
      m_mixer[mixer::SPEAKER]->process_events();
    }
    if (m_mixer[mixer::HEADPHONE]) {
      m_mixer[mixer::HEADPHONE]->process_events();
    }
    if (m_ctrl[control::HEADPHONE]) {
      m_ctrl[control::HEADPHONE]->process_events();
    }

    // Get volume, mute and headphone state
    m_volume = 100;
    m_muted = false;
    m_headphones = false;

    try {
      if (m_mixer[mixer::MASTER]) {
        m_volume = m_volume * (m_mapped ? m_mixer[mixer::MASTER]->get_normalized_volume() / 100.0f
                                        : m_mixer[mixer::MASTER]->get_volume() / 100.0f);
        m_muted = m_muted || m_mixer[mixer::MASTER]->is_muted();
      }
    } catch (const alsa_exception& err) {
      m_log.err("%s: Failed to query master mixer (%s)", name(), err.what());
    }

    try {
      if (m_ctrl[control::HEADPHONE] && m_ctrl[control::HEADPHONE]->test_device_plugged()) {
        m_headphones = true;
        m_volume = m_volume * (m_mapped ? m_mixer[mixer::HEADPHONE]->get_normalized_volume() / 100.0f
                                        : m_mixer[mixer::HEADPHONE]->get_volume() / 100.0f);
        m_muted = m_muted || m_mixer[mixer::HEADPHONE]->is_muted();
      }
    } catch (const alsa_exception& err) {
      m_log.err("%s: Failed to query headphone mixer (%s)", name(), err.what());
    }

    try {
      if (!m_headphones && m_mixer[mixer::SPEAKER]) {
        m_volume = m_volume * (m_mapped ? m_mixer[mixer::SPEAKER]->get_normalized_volume() / 100.0f
                                        : m_mixer[mixer::SPEAKER]->get_volume() / 100.0f);
        m_muted = m_muted || m_mixer[mixer::SPEAKER]->is_muted();
      }
    } catch (const alsa_exception& err) {
      m_log.err("%s: Failed to query speaker mixer (%s)", name(), err.what());
    }

    // Replace label tokens
    if (m_label_volume) {
      m_label_volume->reset_tokens();
      m_label_volume->replace_token("%percentage%", to_string(m_volume));
    }

    if (m_label_muted) {
      m_label_muted->reset_tokens();
      m_label_muted->replace_token("%percentage%", to_string(m_volume));
    }

    return true;
  }

  string alsa_module::get_format() const {
    return m_muted ? FORMAT_MUTED : FORMAT_VOLUME;
  }

  string alsa_module::get_output() {
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
      m_builder->action(mousebtn::SCROLL_UP, *this, EVENT_INC, "");
      m_builder->action(mousebtn::SCROLL_DOWN, *this, EVENT_DEC, "");
    }

    m_builder->node(output);

    return m_builder->flush();
  }

  bool alsa_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR_VOLUME) {
      builder->node(m_bar_volume->output(m_volume));
    } else if (tag == TAG_RAMP_VOLUME && (!m_headphones || !*m_ramp_headphones)) {
      builder->node(m_ramp_volume->get_by_percentage(m_volume));
    } else if (tag == TAG_RAMP_VOLUME && m_headphones && *m_ramp_headphones) {
      builder->node(m_ramp_headphones->get_by_percentage(m_volume));
    } else if (tag == TAG_LABEL_VOLUME) {
      builder->node(m_label_volume);
    } else if (tag == TAG_LABEL_MUTED) {
      builder->node(m_label_muted);
    } else {
      return false;
    }
    return true;
  }

  void alsa_module::action_inc() {
    change_volume(m_interval);
  }

  void alsa_module::action_dec() {
    change_volume(-m_interval);
  }

  void alsa_module::action_toggle() {
    if (!m_mixer[mixer::MASTER]) {
      return;
    }
    const auto& mixers = get_mixers();
    for (auto&& mixer : mixers) {
      mixer->set_mute(m_muted || mixers[0]->is_muted());
    }

    action_epilogue(mixers);
  }

  void alsa_module::change_volume(int interval) {
    if (!m_mixer[mixer::MASTER]) {
      return;
    }
    const auto& mixers = get_mixers();
    for (auto&& mixer : mixers) {
      if (m_unmute_on_scroll) {
        mixer->set_mute(true);
      }
      m_mapped ? mixer->set_normalized_volume(math_util::cap<float>(mixer->get_normalized_volume() + interval, 0, 100))
               : mixer->set_volume(math_util::cap<float>(mixer->get_volume() + interval, 0, 100));
    }
    action_epilogue(mixers);
  }

  void alsa_module::action_epilogue(const vector<mixer_t>& mixers) {
    for (auto&& mixer : mixers) {
      if (mixer->wait(0)) {
        mixer->process_events();
      }
    }
  }

  vector<mixer_t> alsa_module::get_mixers() {
    vector<mixer_t> mixers;
    bool headphones{m_headphones};

    if (m_mixer[mixer::MASTER] && !m_mixer[mixer::MASTER]->get_name().empty()) {
      mixers.emplace_back(std::make_shared<mixer_t::element_type>(
          string{m_mixer[mixer::MASTER]->get_name()}, string{m_mixer[mixer::MASTER]->get_sound_card()}));
    }
    if (m_mixer[mixer::HEADPHONE] && !m_mixer[mixer::HEADPHONE]->get_name().empty() && headphones) {
      mixers.emplace_back(std::make_shared<mixer_t::element_type>(
          string{m_mixer[mixer::HEADPHONE]->get_name()}, string{m_mixer[mixer::HEADPHONE]->get_sound_card()}));
    }
    if (m_mixer[mixer::SPEAKER] && !m_mixer[mixer::SPEAKER]->get_name().empty() && !headphones) {
      mixers.emplace_back(std::make_shared<mixer_t::element_type>(
          string{m_mixer[mixer::SPEAKER]->get_name()}, string{m_mixer[mixer::SPEAKER]->get_sound_card()}));
    }

    return mixers;
  }
} // namespace modules

POLYBAR_NS_END

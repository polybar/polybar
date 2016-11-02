#include "modules/volume.hpp"
#include "utils/math.hpp"

LEMONBUDDY_NS

namespace modules {
  void volume_module::setup() {
    // Load configuration values {{{

    string master_mixer_name{"Master"};
    string speaker_mixer_name;
    string headphone_mixer_name;

    GET_CONFIG_VALUE(name(), master_mixer_name, "master-mixer");
    GET_CONFIG_VALUE(name(), speaker_mixer_name, "speaker-mixer");
    GET_CONFIG_VALUE(name(), headphone_mixer_name, "headphone-mixer");

    if (!headphone_mixer_name.empty())
      REQ_CONFIG_VALUE(name(), m_headphoneid, "headphone-id");

    if (string_util::compare(speaker_mixer_name, "master"))
      throw module_error("Master mixer is already defined");
    if (string_util::compare(headphone_mixer_name, "master"))
      throw module_error("Master mixer is already defined");

    // }}}
    // Setup mixers {{{

    try {
      if (!master_mixer_name.empty())
        m_mixers[mixer::MASTER].reset(new mixer_t::element_type{master_mixer_name});
      if (!speaker_mixer_name.empty())
        m_mixers[mixer::SPEAKER].reset(new mixer_t::element_type{speaker_mixer_name});
      if (!headphone_mixer_name.empty())
        m_mixers[mixer::HEADPHONE].reset(new mixer_t::element_type{headphone_mixer_name});
      if (m_mixers[mixer::HEADPHONE])
        m_controls[control::HEADPHONE].reset(new control_t::element_type{m_headphoneid});
      if (m_mixers.empty())
        throw module_error("No configured mixers");
    } catch (const alsa_mixer_error& err) {
      throw module_error(err.what());
    } catch (const alsa_ctl_interface_error& err) {
      throw module_error(err.what());
    }

    // }}}
    // Add formats and elements {{{

    m_formatter->add(
        FORMAT_VOLUME, TAG_LABEL_VOLUME, {TAG_RAMP_VOLUME, TAG_LABEL_VOLUME, TAG_BAR_VOLUME});
    m_formatter->add(
        FORMAT_MUTED, TAG_LABEL_MUTED, {TAG_RAMP_VOLUME, TAG_LABEL_MUTED, TAG_BAR_VOLUME});

    if (m_formatter->has(TAG_BAR_VOLUME))
      m_bar_volume = load_progressbar(m_bar, m_conf, name(), TAG_BAR_VOLUME);
    if (m_formatter->has(TAG_LABEL_VOLUME, FORMAT_VOLUME))
      m_label_volume = load_optional_label(m_conf, name(), TAG_LABEL_VOLUME, "%percentage%");
    if (m_formatter->has(TAG_LABEL_MUTED, FORMAT_MUTED))
      m_label_muted = load_optional_label(m_conf, name(), TAG_LABEL_MUTED, "%percentage%");
    if (m_formatter->has(TAG_RAMP_VOLUME)) {
      m_ramp_volume = load_ramp(m_conf, name(), TAG_RAMP_VOLUME);
      m_ramp_headphones = load_ramp(m_conf, name(), TAG_RAMP_HEADPHONES, false);
    }

    // }}}
  }

  void volume_module::teardown() {
    m_mixers.clear();
  }

  bool volume_module::has_event() {
    // Poll for mixer and control events {{{

    try {
      if (m_mixers[mixer::MASTER] && m_mixers[mixer::MASTER]->wait(25))
        return true;
      if (m_mixers[mixer::SPEAKER] && m_mixers[mixer::SPEAKER]->wait(25))
        return true;
      if (m_mixers[mixer::HEADPHONE] && m_mixers[mixer::HEADPHONE]->wait(25))
        return true;
      if (m_controls[control::HEADPHONE] && m_controls[control::HEADPHONE]->wait(25))
        return true;
    } catch (const alsa_exception& e) {
      m_log.err("%s: %s", name(), e.what());
    }

    return false;

    // }}}
  }

  bool volume_module::update() {
    // Consume pending events {{{

    if (m_mixers[mixer::MASTER])
      m_mixers[mixer::MASTER]->process_events();
    if (m_mixers[mixer::SPEAKER])
      m_mixers[mixer::SPEAKER]->process_events();
    if (m_mixers[mixer::HEADPHONE])
      m_mixers[mixer::HEADPHONE]->process_events();
    if (m_controls[control::HEADPHONE])
      m_controls[control::HEADPHONE]->process_events();

    // }}}
    // Get volume, mute and headphone state {{{

    m_volume = 100;
    m_muted = false;
    m_headphones = false;

    if (m_mixers[mixer::MASTER]) {
      m_volume = m_volume * m_mixers[mixer::MASTER]->get_volume() / 100.0f;
      m_muted = m_muted || m_mixers[mixer::MASTER]->is_muted();
    }

    if (m_controls[control::HEADPHONE] && m_controls[control::HEADPHONE]->test_device_plugged()) {
      m_headphones = true;
      m_volume = m_volume * m_mixers[mixer::HEADPHONE]->get_volume() / 100.0f;
      m_muted = m_muted || m_mixers[mixer::HEADPHONE]->is_muted();
    } else if (m_mixers[mixer::SPEAKER]) {
      m_volume = m_volume * m_mixers[mixer::SPEAKER]->get_volume() / 100.0f;
      m_muted = m_muted || m_mixers[mixer::SPEAKER]->is_muted();
    }

    // }}}
    // Replace label tokens {{{

    if (m_label_volume) {
      m_label_volume->reset_tokens();
      m_label_volume->replace_token("%percentage%", to_string(m_volume) + "%");
    }

    if (m_label_muted) {
      m_label_muted->reset_tokens();
      m_label_muted->replace_token("%percentage%", to_string(m_volume) + "%");
    }

    // }}}

    return true;
  }

  string volume_module::get_format() const {
    return m_muted ? FORMAT_MUTED : FORMAT_VOLUME;
  }

  string volume_module::get_output() {
    m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE_MUTE);

    if (!m_muted && m_volume < 100)
      m_builder->cmd(mousebtn::SCROLL_UP, EVENT_VOLUME_UP);
    if (!m_muted && m_volume > 0)
      m_builder->cmd(mousebtn::SCROLL_DOWN, EVENT_VOLUME_DOWN);

    m_builder->node(module::get_output());

    return m_builder->flush();
  }

  bool volume_module::build(builder* builder, string tag) const {
    if (tag == TAG_BAR_VOLUME)
      builder->node(m_bar_volume->output(m_volume));
    else if (tag == TAG_RAMP_VOLUME && (!m_headphones || !*m_ramp_headphones))
      builder->node(m_ramp_volume->get_by_percentage(m_volume));
    else if (tag == TAG_RAMP_VOLUME && m_headphones && *m_ramp_headphones)
      builder->node(m_ramp_headphones->get_by_percentage(m_volume));
    else if (tag == TAG_LABEL_VOLUME)
      builder->node(m_label_volume);
    else if (tag == TAG_LABEL_MUTED)
      builder->node(m_label_muted);
    else
      return false;
    return true;
  }

  bool volume_module::handle_event(string cmd) {
    if (cmd.compare(0, 3, EVENT_PREFIX) != 0)
      return false;
    if (!m_mixers[mixer::MASTER])
      return false;

    vector<mixer_t> mixers;

    if (m_mixers[mixer::MASTER])
      mixers.emplace_back(new mixer_t::element_type(m_mixers[mixer::MASTER]->get_name()));
    if (m_mixers[mixer::HEADPHONE] && m_headphones)
      mixers.emplace_back(new mixer_t::element_type(m_mixers[mixer::HEADPHONE]->get_name()));
    if (m_mixers[mixer::SPEAKER] && !m_headphones)
      mixers.emplace_back(new mixer_t::element_type(m_mixers[mixer::SPEAKER]->get_name()));

    try {
      if (cmd.compare(0, strlen(EVENT_TOGGLE_MUTE), EVENT_TOGGLE_MUTE) == 0) {
        for (auto&& mixer : mixers) {
          mixer->set_mute(m_muted || mixers[0]->is_muted());
        }
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_UP), EVENT_VOLUME_UP) == 0) {
        for (auto&& mixer : mixers) {
          mixer->set_volume(math_util::cap<float>(mixer->get_volume() + 5, 0, 100));
        }
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_DOWN), EVENT_VOLUME_DOWN) == 0) {
        for (auto&& mixer : mixers) {
          mixer->set_volume(math_util::cap<float>(mixer->get_volume() - 5, 0, 100));
        }
      } else {
        return false;
      }
    } catch (const std::exception& err) {
      m_log.err("%s: Failed to handle command (%s)", name(), err.what());
    }

    return true;
  }

  bool volume_module::receive_events() const {
    return true;
  }
}

LEMONBUDDY_NS_END

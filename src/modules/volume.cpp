#include "modules/volume.hpp"
#include "adapters/alsa/control.hpp"
#include "adapters/alsa/generic.hpp"
#include "adapters/alsa/mixer.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/math.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/event_module.inl"

POLYBAR_NS

using namespace alsa;

namespace modules {
  template class module<volume_module>;
  template class event_module<volume_module>;

  void volume_module::setup() {
    // Load configuration values
    string master_mixer_name{"Master"};
    string speaker_mixer_name;
    string headphone_mixer_name;

    GET_CONFIG_VALUE(name(), master_mixer_name, "master-mixer");
    GET_CONFIG_VALUE(name(), speaker_mixer_name, "speaker-mixer");
    GET_CONFIG_VALUE(name(), headphone_mixer_name, "headphone-mixer");
    GET_CONFIG_VALUE(name(), m_mapped, "mapped");

    if (!headphone_mixer_name.empty()) {
      REQ_CONFIG_VALUE(name(), m_headphoneid, "headphone-id");
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
        m_mixer[mixer::MASTER].reset(new mixer_t::element_type{master_mixer_name});
      }
      if (!speaker_mixer_name.empty()) {
        m_mixer[mixer::SPEAKER].reset(new mixer_t::element_type{speaker_mixer_name});
      }
      if (!headphone_mixer_name.empty()) {
        m_mixer[mixer::HEADPHONE].reset(new mixer_t::element_type{headphone_mixer_name});
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
      m_label_volume = load_optional_label(m_conf, name(), TAG_LABEL_VOLUME, "%percentage%");
    }
    if (m_formatter->has(TAG_LABEL_MUTED, FORMAT_MUTED)) {
      m_label_muted = load_optional_label(m_conf, name(), TAG_LABEL_MUTED, "%percentage%");
    }
    if (m_formatter->has(TAG_RAMP_VOLUME)) {
      m_ramp_volume = load_ramp(m_conf, name(), TAG_RAMP_VOLUME);
      m_ramp_headphones = load_ramp(m_conf, name(), TAG_RAMP_HEADPHONES, false);
    }
  }

  void volume_module::teardown() {
    m_mixer.clear();
    m_ctrl.clear();
  }

  bool volume_module::has_event() {
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

  bool volume_module::update() {
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
      m_label_volume->replace_token("%percentage%", to_string(m_volume) + "%");
    }

    if (m_label_muted) {
      m_label_muted->reset_tokens();
      m_label_muted->replace_token("%percentage%", to_string(m_volume) + "%");
    }

    return true;
  }

  string volume_module::get_format() const {
    return m_muted ? FORMAT_MUTED : FORMAT_VOLUME;
  }

  string volume_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapper
    // with the cmd handlers
    string output{module::get_output()};

    m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE_MUTE);
    m_builder->cmd(mousebtn::SCROLL_UP, EVENT_VOLUME_UP, !m_muted && m_volume < 100);
    m_builder->cmd(mousebtn::SCROLL_DOWN, EVENT_VOLUME_DOWN, !m_muted && m_volume > 0);

    m_builder->append(output);

    return m_builder->flush();
  }

  bool volume_module::build(builder* builder, const string& tag) const {
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

  bool volume_module::handle_event(string cmd) {
    if (cmd.compare(0, 3, EVENT_PREFIX) != 0) {
      return false;
    }

    if (!m_mixer[mixer::MASTER]) {
      return false;
    }

    try {
      vector<mixer_t> mixers;
      bool headphones{m_headphones};

      if (m_mixer[mixer::MASTER] && !m_mixer[mixer::MASTER]->get_name().empty()) {
        mixers.emplace_back(new mixer_t::element_type(m_mixer[mixer::MASTER]->get_name()));
      }
      if (m_mixer[mixer::HEADPHONE] && !m_mixer[mixer::HEADPHONE]->get_name().empty() && headphones) {
        mixers.emplace_back(new mixer_t::element_type(m_mixer[mixer::HEADPHONE]->get_name()));
      }
      if (m_mixer[mixer::SPEAKER] && !m_mixer[mixer::SPEAKER]->get_name().empty() && !headphones) {
        mixers.emplace_back(new mixer_t::element_type(m_mixer[mixer::SPEAKER]->get_name()));
      }

      if (cmd.compare(0, strlen(EVENT_TOGGLE_MUTE), EVENT_TOGGLE_MUTE) == 0) {
        for (auto&& mixer : mixers) {
          mixer->set_mute(m_muted || mixers[0]->is_muted());
        }
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_UP), EVENT_VOLUME_UP) == 0) {
        for (auto&& mixer : mixers) {
          m_mapped ? mixer->set_normalized_volume(math_util::cap<float>(mixer->get_normalized_volume() + 5, 0, 100))
                   : mixer->set_volume(math_util::cap<float>(mixer->get_volume() + 5, 0, 100));
        }
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_DOWN), EVENT_VOLUME_DOWN) == 0) {
        for (auto&& mixer : mixers) {
          m_mapped ? mixer->set_normalized_volume(math_util::cap<float>(mixer->get_normalized_volume() - 5, 0, 100))
                   : mixer->set_volume(math_util::cap<float>(mixer->get_volume() - 5, 0, 100));
        }
      } else {
        return false;
      }

      for (auto&& mixer : mixers) {
        if (mixer->wait(0)) {
          mixer->process_events();
        }
      }
    } catch (const exception& err) {
      m_log.err("%s: Failed to handle command (%s)", name(), err.what());
    }

    return true;
  }

  bool volume_module::receive_events() const {
    return true;
  }
}

POLYBAR_NS_END

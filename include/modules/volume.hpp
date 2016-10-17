#pragma once

#include "adapters/alsa.hpp"
#include "components/config.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  enum class mixer { NONE = 0, MASTER, SPEAKER, HEADPHONE };
  enum class control { NONE = 0, HEADPHONE };

  using mixer_t = shared_ptr<alsa_mixer>;
  using control_t = shared_ptr<alsa_ctl_interface>;

  class volume_module : public event_module<volume_module> {
   public:
    using event_module::event_module;

    void setup() {
      // Load configuration values {{{

      auto master_mixer = m_conf.get<string>(name(), "master-mixer", "Master");
      auto speaker_mixer = m_conf.get<string>(name(), "speaker-mixer", "");
      auto headphone_mixer = m_conf.get<string>(name(), "headphone-mixer", "");

      GET_CONFIG_VALUE(name(), m_headphoneid, "headphone-id");

      if (!headphone_mixer.empty() && m_headphoneid == -1)
        throw module_error(
            "volume_module: Missing required property value for \"headphone-id\"...");
      else if (headphone_mixer.empty() && m_headphoneid != -1)
        throw module_error(
            "volume_module: Missing required property value for \"headphone-mixer\"...");

      if (string_util::lower(speaker_mixer) == "master")
        throw module_error(
            "volume_module: The \"Master\" mixer is already processed internally. Specify another "
            "mixer or comment out the \"speaker-mixer\" parameter...");
      if (string_util::lower(headphone_mixer) == "master")
        throw module_error(
            "volume_module: The \"Master\" mixer is already processed internally. Specify another "
            "mixer or comment out the \"headphone-mixer\" parameter...");

      // }}}
      // Setup mixers {{{

      auto create_mixer = [this](string mixer_name) {
        try {
          return mixer_t{new mixer_t::element_type{mixer_name}};
        } catch (const alsa_mixer_error& e) {
          m_log.err("%s: Failed to open '%s' mixer => %s", name(), mixer_name, e.what());
          return mixer_t{};
        }
      };

      m_mixers[mixer::MASTER] = create_mixer(master_mixer);

      if (!speaker_mixer.empty())
        m_mixers[mixer::SPEAKER] = create_mixer(speaker_mixer);
      if (!headphone_mixer.empty())
        m_mixers[mixer::HEADPHONE] = create_mixer(headphone_mixer);

      if (m_mixers.empty()) {
        m_log.err("%s: No configured mixers, stopping module...", name());
        stop();
        return;
      }

      if (m_mixers[mixer::HEADPHONE] && m_headphoneid > -1) {
        try {
          m_controls[control::HEADPHONE] = control_t{new control_t::element_type{m_headphoneid}};
        } catch (const alsa_ctl_interface_error& e) {
          m_log.err("%s: Failed to open headphone control interface => %s", name(), e.what());
          m_controls[control::HEADPHONE].reset();
        }
      }

      // }}}
      // Add formats and elements {{{

      m_formatter->add(
          FORMAT_VOLUME, TAG_LABEL_VOLUME, {TAG_RAMP_VOLUME, TAG_LABEL_VOLUME, TAG_BAR_VOLUME});
      m_formatter->add(
          FORMAT_MUTED, TAG_LABEL_MUTED, {TAG_RAMP_VOLUME, TAG_LABEL_MUTED, TAG_BAR_VOLUME});

      if (m_formatter->has(TAG_BAR_VOLUME)) {
        m_bar_volume = get_config_bar(m_bar, m_conf, name(), TAG_BAR_VOLUME);
      }
      if (m_formatter->has(TAG_RAMP_VOLUME)) {
        m_ramp_volume = get_config_ramp(m_conf, name(), TAG_RAMP_VOLUME);
        m_ramp_headphones = get_config_ramp(m_conf, name(), TAG_RAMP_HEADPHONES, false);
      }
      if (m_formatter->has(TAG_LABEL_VOLUME, FORMAT_VOLUME)) {
        m_label_volume =
            get_optional_config_label(m_conf, name(), TAG_LABEL_VOLUME, "%percentage%");
        m_label_volume_tokenized = m_label_volume->clone();
      }
      if (m_formatter->has(TAG_LABEL_MUTED, FORMAT_MUTED)) {
        m_label_muted = get_optional_config_label(m_conf, name(), TAG_LABEL_MUTED, "%percentage%");
        m_label_muted_tokenized = m_label_muted->clone();
      }

      // }}}
    }

    void stop() {
      // Deconstruct all mixers before putting the module in its stopped state {{{

      std::lock_guard<threading_util::spin_lock> lck(this->update_lock);
      m_mixers[mixer::MASTER].reset();
      m_mixers[mixer::SPEAKER].reset();
      m_mixers[mixer::HEADPHONE].reset();
      m_controls[control::HEADPHONE].reset();
      event_module::stop();

      // }}}
    }

    bool has_event() {
      // Poll for mixer and control events {{{

      try {
        bool has_event = false;

        if (m_mixers[mixer::MASTER])
          has_event |= m_mixers[mixer::MASTER]->wait(25);
        if (m_mixers[mixer::SPEAKER])
          has_event |= m_mixers[mixer::SPEAKER]->wait(25);
        if (m_mixers[mixer::HEADPHONE])
          has_event |= m_mixers[mixer::HEADPHONE]->wait(25);
        if (m_controls[control::HEADPHONE])
          has_event |= m_controls[control::HEADPHONE]->wait(25);

        return has_event;
      } catch (const alsa_exception& e) {
        m_log.err("%s: %s", name(), e.what());
        return false;
      }

      // }}}
    }

    bool update() {
      // Consume pending events {{{

      if (m_mixers[mixer::MASTER])
        m_mixers[mixer::MASTER]->process_events();
      if (m_mixers[mixer::SPEAKER])
        m_mixers[mixer::SPEAKER]->process_events();
      if (m_mixers[mixer::HEADPHONE])
        m_mixers[mixer::HEADPHONE]->process_events();
      if (m_controls[control::HEADPHONE])
        m_controls[control::HEADPHONE]->wait(0);

      // }}}
      // Get volume, mute and headphone state {{{

      m_volume = 100;
      m_muted = false;
      m_headphones = false;

      if (m_mixers[mixer::MASTER]) {
        m_volume *= m_mixers[mixer::MASTER]->get_volume() / 100.0f;
        m_muted = m_muted || m_mixers[mixer::MASTER]->is_muted();
      }

      if (m_controls[control::HEADPHONE] && m_controls[control::HEADPHONE]->test_device_plugged()) {
        m_headphones = true;
        m_volume *= m_mixers[mixer::HEADPHONE]->get_volume() / 100.0f;
        m_muted = m_muted || m_mixers[mixer::HEADPHONE]->is_muted();
      } else if (m_mixers[mixer::SPEAKER]) {
        m_volume *= m_mixers[mixer::SPEAKER]->get_volume() / 100.0f;
        m_muted = m_muted || m_mixers[mixer::SPEAKER]->is_muted();
      }

      // }}}
      // Replace label tokens {{{

      if (m_label_volume_tokenized) {
        m_label_volume_tokenized->m_text = m_label_volume->m_text;
        m_label_volume_tokenized->replace_token("%percentage%", to_string(m_volume) + "%");
      }
      if (m_label_muted_tokenized) {
        m_label_muted_tokenized->m_text = m_label_muted->m_text;
        m_label_muted_tokenized->replace_token("%percentage%", to_string(m_volume) + "%");
      }

      // }}}

      return true;
    }

    string get_format() {
      return m_muted ? FORMAT_MUTED : FORMAT_VOLUME;
    }

    string get_output() {
      m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE_MUTE);

      if (!m_muted && m_volume < 100)
        m_builder->cmd(mousebtn::SCROLL_UP, EVENT_VOLUME_UP);
      if (!m_muted && m_volume > 0)
        m_builder->cmd(mousebtn::SCROLL_DOWN, EVENT_VOLUME_DOWN);

      m_builder->node(module::get_output());
      return m_builder->flush();
    }

    bool build(builder* builder, string tag) {
      if (tag == TAG_BAR_VOLUME)
        builder->node(m_bar_volume->output(m_volume));
      else if (tag == TAG_RAMP_VOLUME && (!m_headphones || !*m_ramp_headphones))
        builder->node(m_ramp_volume->get_by_percentage(m_volume));
      else if (tag == TAG_RAMP_VOLUME && m_headphones && *m_ramp_headphones)
        builder->node(m_ramp_headphones->get_by_percentage(m_volume));
      else if (tag == TAG_LABEL_VOLUME)
        builder->node(m_label_volume_tokenized);
      else if (tag == TAG_LABEL_MUTED)
        builder->node(m_label_muted_tokenized);
      else
        return false;
      return true;
    }

    bool handle_event(string cmd) {
      if (cmd.compare(0, 3, EVENT_PREFIX) != 0)
        return false;
      if (!m_mixers[mixer::MASTER])
        return false;

      vector<mixer_t> mixers{m_mixers[mixer::MASTER]};

      if (m_mixers[mixer::HEADPHONE] && m_headphones)
        mixers.emplace_back(m_mixers[mixer::HEADPHONE]);
      else if (m_mixers[mixer::SPEAKER])
        mixers.emplace_back(m_mixers[mixer::SPEAKER]);

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

      // Update the mute flag since we won't poll the new state when
      // sending the broadcast related to this event
      m_muted = !m_muted;

      event_handled();

      return true;
    }

    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto FORMAT_VOLUME = "format-volume";
    static constexpr auto FORMAT_MUTED = "format-muted";

    static constexpr auto TAG_RAMP_VOLUME = "<ramp-volume>";
    static constexpr auto TAG_RAMP_HEADPHONES = "<ramp-headphones>";
    static constexpr auto TAG_BAR_VOLUME = "<bar-volume>";
    static constexpr auto TAG_LABEL_VOLUME = "<label-volume>";
    static constexpr auto TAG_LABEL_MUTED = "<label-muted>";

    static constexpr auto EVENT_PREFIX = "vol";
    static constexpr auto EVENT_VOLUME_UP = "volup";
    static constexpr auto EVENT_VOLUME_DOWN = "voldown";
    static constexpr auto EVENT_TOGGLE_MUTE = "volmute";

    progressbar_t m_bar_volume;
    ramp_t m_ramp_volume;
    ramp_t m_ramp_headphones;
    label_t m_label_volume;
    label_t m_label_volume_tokenized;
    label_t m_label_muted;
    label_t m_label_muted_tokenized;

    map<mixer, mixer_t> m_mixers;
    map<control, control_t> m_controls;

    int m_headphoneid = -1;
    int m_volume = 0;

    stateflag m_muted{false};
    stateflag m_headphones{false};
  };
}

LEMONBUDDY_NS_END

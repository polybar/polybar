#pragma once

#include "adapters/alsa.hpp"
#include "config.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "modules/meta.hpp"

LEMONBUDDY_NS

namespace modules {
  class volume_module : public event_module<volume_module> {
   public:
    using event_module::event_module;

    void setup() {
      // Load configuration values {{{

      auto master_mixer = m_conf.get<string>(name(), "master-mixer", "Master");
      auto speaker_mixer = m_conf.get<string>(name(), "speaker-mixer", "");
      auto headphone_mixer = m_conf.get<string>(name(), "headphone-mixer", "");

      m_headphone_ctrl_numid = m_conf.get<int>(name(), "headphone-id", -1);

      if (!headphone_mixer.empty() && m_headphone_ctrl_numid == -1)
        throw module_error(
            "volume_module: Missing required property value for \"headphone-id\"...");
      else if (headphone_mixer.empty() && m_headphone_ctrl_numid != -1)
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

      auto create_mixer = [](const logger& log, string mixer_name) {
        unique_ptr<alsa_mixer> mixer;

        try {
          mixer = make_unique<alsa_mixer>(mixer_name);
        } catch (const alsa_mixer_error& e) {
          log.err("volume_module: Failed to open '%s' mixer => %s", mixer_name, e.what());
          mixer.reset();
        }

        return mixer;
      };

      m_master_mixer = create_mixer(m_log, master_mixer);

      if (!speaker_mixer.empty())
        m_speaker_mixer = create_mixer(m_log, speaker_mixer);
      if (!headphone_mixer.empty())
        m_headphone_mixer = create_mixer(m_log, headphone_mixer);

      if (!m_master_mixer && !m_speaker_mixer && !m_headphone_mixer) {
        stop();
        return;
      }

      if (m_headphone_mixer && m_headphone_ctrl_numid > -1) {
        try {
          m_headphone_ctrl = make_unique<alsa_ctl_interface>(m_headphone_ctrl_numid);
        } catch (const alsa_ctl_interface_error& e) {
          m_log.err("%s: Failed to open headphone control interface => %s", name(), e.what());
          m_headphone_ctrl.reset();
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
      std::lock_guard<threading_util::spin_lock> lck(this->update_lock);
      m_master_mixer.reset();
      m_speaker_mixer.reset();
      m_headphone_mixer.reset();
      m_headphone_ctrl.reset();
      event_module::stop();
    }

    bool has_event() {
      try {
        bool has_event = false;
        if (m_master_mixer)
          has_event |= m_master_mixer->wait(25);
        if (m_speaker_mixer)
          has_event |= m_speaker_mixer->wait(25);
        if (m_headphone_mixer)
          has_event |= m_headphone_mixer->wait(25);
        if (m_headphone_ctrl)
          has_event |= m_headphone_ctrl->wait(25);
        return has_event;
      } catch (const alsa_exception& e) {
        m_log.err("%s: %s", name(), e.what());
        return false;
      }
    }

    bool update() {
      // Consume any other pending events
      if (m_master_mixer)
        m_master_mixer->process_events();
      if (m_speaker_mixer)
        m_speaker_mixer->process_events();
      if (m_headphone_mixer)
        m_headphone_mixer->process_events();
      if (m_headphone_ctrl)
        m_headphone_ctrl->wait(0);

      int volume = 100;
      bool muted = false;

      if (m_master_mixer) {
        volume *= m_master_mixer->get_volume() / 100.0f;
        muted |= m_master_mixer->is_muted();
      }

      if (m_headphone_mixer && m_headphone_ctrl && m_headphone_ctrl->test_device_plugged()) {
        m_headphones = true;
        volume *= m_headphone_mixer->get_volume() / 100.0f;
        muted |= m_headphone_mixer->is_muted();
      } else if (m_speaker_mixer) {
        m_headphones = false;
        volume *= m_speaker_mixer->get_volume() / 100.0f;
        muted |= m_speaker_mixer->is_muted();
      }

      m_volume = volume;
      m_muted = muted;

      if (m_label_volume_tokenized) {
        m_label_volume_tokenized->m_text = m_label_volume->m_text;
        m_label_volume_tokenized->replace_token("%percentage%", to_string(m_volume) + "%");
      }

      if (m_label_muted_tokenized) {
        m_label_muted_tokenized->m_text = m_label_muted->m_text;
        m_label_muted_tokenized->replace_token("%percentage%", to_string(m_volume) + "%");
      }

      return true;
    }

    string get_format() {
      return m_muted == true ? FORMAT_MUTED : FORMAT_VOLUME;
    }

    string get_output() {
      m_builder->cmd(mousebtn::LEFT, EVENT_TOGGLE_MUTE);

      if (!m_muted && m_volume < 100)
        m_builder->cmd(mousebtn::SCROLL_UP, EVENT_VOLUME_UP);
      else
        m_log.trace("%s: Not adding scroll up handler (muted or volume = 100)", name());

      if (!m_muted && m_volume > 0)
        m_builder->cmd(mousebtn::SCROLL_DOWN, EVENT_VOLUME_DOWN);
      else
        m_log.trace("%s: Not adding scroll down handler (muted or volume = 0)", name());

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

      if (!m_master_mixer)
        return false;

      alsa_mixer* master_mixer = m_master_mixer.get();
      alsa_mixer* other_mixer = nullptr;

      if (m_headphone_mixer && m_headphones)
        other_mixer = m_headphone_mixer.get();
      else if (m_speaker_mixer)
        other_mixer = m_speaker_mixer.get();
      else
        return false;

      if (cmd.compare(0, strlen(EVENT_TOGGLE_MUTE), EVENT_TOGGLE_MUTE) == 0) {
        master_mixer->set_mute(m_muted);
        if (other_mixer != nullptr)
          other_mixer->set_mute(m_muted);
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_UP), EVENT_VOLUME_UP) == 0) {
        master_mixer->set_volume(math_util::cap<float>(master_mixer->get_volume() + 5, 0, 100));
        if (other_mixer != nullptr)
          other_mixer->set_volume(math_util::cap<float>(other_mixer->get_volume() + 5, 0, 100));
      } else if (cmd.compare(0, strlen(EVENT_VOLUME_DOWN), EVENT_VOLUME_DOWN) == 0) {
        master_mixer->set_volume(math_util::cap<float>(master_mixer->get_volume() - 5, 0, 100));
        if (other_mixer != nullptr)
          other_mixer->set_volume(math_util::cap<float>(other_mixer->get_volume() - 5, 0, 100));
      } else {
        return false;
      }

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

    unique_ptr<alsa_mixer> m_master_mixer;
    unique_ptr<alsa_mixer> m_speaker_mixer;
    unique_ptr<alsa_mixer> m_headphone_mixer;
    unique_ptr<alsa_ctl_interface> m_headphone_ctrl;
    int m_headphone_ctrl_numid = -1;
    int m_volume = 0;

    stateflag m_muted{false};
    stateflag m_headphones{false};
  };
}

LEMONBUDDY_NS_END

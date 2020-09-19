#include <cmath>

#include "adapters/alsa/generic.hpp"
#include "adapters/alsa/mixer.hpp"
#include "utils/math.hpp"

#define MAX_LINEAR_DB_SCALE 24

POLYBAR_NS

namespace alsa {
  /**
   * Construct mixer object
   */
  mixer::mixer(string&& mixer_selem_name, string&& soundcard_name)
      : m_name(forward<string>(mixer_selem_name)), s_name(soundcard_name) {
    int err = 0;

    if ((err = snd_mixer_open(&m_mixer, 1)) == -1) {
      throw_exception<mixer_error>("Failed to open hardware mixer", err);
    }

    snd_config_update_free_global();

    if ((err = snd_mixer_attach(m_mixer, s_name.c_str())) == -1) {
      throw_exception<mixer_error>("Failed to attach hardware mixer control", err);
    }
    if ((err = snd_mixer_selem_register(m_mixer, nullptr, nullptr)) == -1) {
      throw_exception<mixer_error>("Failed to register simple mixer element", err);
    }
    if ((err = snd_mixer_load(m_mixer)) == -1) {
      throw_exception<mixer_error>("Failed to load mixer", err);
    }

    snd_mixer_selem_id_t* sid{nullptr};
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, m_name.c_str());

    if ((m_elem = snd_mixer_find_selem(m_mixer, sid)) == nullptr) {
      throw mixer_error("Cannot find simple element");
    }
  }

  /**
   * Deconstruct mixer
   */
  mixer::~mixer() {
    if (m_mixer != nullptr) {
      snd_mixer_close(m_mixer);
    }
  }

  /**
   * Get mixer name
   */
  const string& mixer::get_name() {
    return m_name;
  }

  /**
   * Get the name of the soundcard that is associated with the mixer
   */
  const string& mixer::get_sound_card() {
    return s_name;
  }

  /**
   * Wait for events
   */
  bool mixer::wait(int timeout) {
    assert(m_mixer);

    int err = 0;

    if ((err = snd_mixer_wait(m_mixer, timeout)) == -1) {
      throw_exception<mixer_error>("Failed to wait for events", err);
    }

    return process_events() > 0;
  }

  /**
   * Process queued mixer events
   */
  int mixer::process_events() {
    int num_events{0};
    if ((num_events = snd_mixer_handle_events(m_mixer)) == -1) {
      throw_exception<mixer_error>("Failed to process pending events", num_events);
    }

    return num_events;
  }

  /**
   * Get volume in percentage
   */
  int mixer::get_volume() {
    assert(m_elem != nullptr);

    long chan_n = 0, vol_total = 0, vol, vol_min, vol_max;

    snd_mixer_selem_get_playback_volume_range(m_elem, &vol_min, &vol_max);

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        snd_mixer_selem_get_playback_volume(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i), &vol);
        vol_total += vol;
        chan_n++;
      }
    }

    return math_util::unbounded_percentage(vol_total / chan_n, vol_min, vol_max);
  }

  /**
   * Get normalized volume in percentage
   */
  int mixer::get_normalized_volume() {
    assert(m_elem != nullptr);

    long chan_n = 0, vol_total = 0, vol, vol_min, vol_max;
    double normalized, min_norm;

    snd_mixer_selem_get_playback_dB_range(m_elem, &vol_min, &vol_max);

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        snd_mixer_selem_get_playback_dB(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i), &vol);
        vol_total += vol;
        chan_n++;
      }
    }

    if (vol_max - vol_min <= MAX_LINEAR_DB_SCALE * 100) {
       return math_util::percentage(vol_total / chan_n, vol_min, vol_max);
    }

    normalized = pow(10, (vol_total / chan_n - vol_max) / 6000.0);
    if (vol_min != SND_CTL_TLV_DB_GAIN_MUTE) {
      min_norm = pow(10, (vol_min - vol_max) / 6000.0);
      normalized = (normalized - min_norm) / (1 - min_norm);
    }

    return 100.0f * normalized + 0.5f;
  }

  /**
   * Set volume to given percentage
   */
  void mixer::set_volume(float percentage) {
    assert(m_elem != nullptr);

    if (is_muted()) {
      return;
    }

    long vol_min, vol_max;
    snd_mixer_selem_get_playback_volume_range(m_elem, &vol_min, &vol_max);
    snd_mixer_selem_set_playback_volume_all(m_elem, math_util::percentage_to_value<int>(percentage, vol_min, vol_max));
  }

  /**
   * Set normalized volume to given percentage
   */
  void mixer::set_normalized_volume(float percentage) {
    assert(m_elem != nullptr);

    if (is_muted()) {
      return;
    }

    long vol_min, vol_max;
    double min_norm;
    percentage = percentage / 100.0f;

    snd_mixer_selem_get_playback_dB_range(m_elem, &vol_min, &vol_max);

    if (vol_max - vol_min <= MAX_LINEAR_DB_SCALE * 100) {
      snd_mixer_selem_set_playback_dB_all(m_elem, lrint(percentage * (vol_max - vol_min)) + vol_min, 0);
      return;
    }

    if (vol_min != SND_CTL_TLV_DB_GAIN_MUTE) {
      min_norm = pow(10, (vol_min - vol_max) / 6000.0);
      percentage = percentage * (1 - min_norm) + min_norm;
    }

    snd_mixer_selem_set_playback_dB_all(m_elem, lrint(6000.0 * log10(percentage)) + vol_max, 0);
  }

  /**
   * Set mute state
   */
  void mixer::set_mute(bool mode) {
    assert(m_elem != nullptr);

    snd_mixer_selem_set_playback_switch_all(m_elem, mode);
  }

  /**
   * Toggle mute state
   */
  void mixer::toggle_mute() {
    assert(m_elem != nullptr);

    int state;

    snd_mixer_selem_get_playback_switch(m_elem, SND_MIXER_SCHN_MONO, &state);
    snd_mixer_selem_set_playback_switch_all(m_elem, !state);
  }

  /**
   * Get current mute state
   */
  bool mixer::is_muted() {
    assert(m_elem != nullptr);

    int state = 0;

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        int state_ = 0;
        snd_mixer_selem_get_playback_switch(m_elem, static_cast<snd_mixer_selem_channel_id_t>(i), &state_);
        state = state || state_;
      }
    }
    return !state;
  }
}

POLYBAR_NS_END

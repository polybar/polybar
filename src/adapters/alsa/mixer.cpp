#include <math.h>

#include "adapters/alsa/generic.hpp"
#include "adapters/alsa/mixer.hpp"
#include "utils/math.hpp"

#define MAX_LINEAR_DB_SCALE 24

POLYBAR_NS

namespace alsa {
  /**
   * Construct mixer object
   */
  mixer::mixer(string mixer_control_name) : m_name(move(mixer_control_name)) {
    if (m_name.empty()) {
      throw mixer_error("Invalid control name");
    }

    snd_mixer_selem_id_malloc(&m_mixerid);

    if (m_mixerid == nullptr) {
      throw mixer_error("Failed to allocate mixer id");
    }

    int err = 0;
    if ((err = snd_mixer_open(&m_hardwaremixer, 1)) == -1) {
      throw_exception<mixer_error>("Failed to open hardware mixer", err);
    }

    snd_config_update_free_global();

    if ((err = snd_mixer_attach(m_hardwaremixer, ALSA_SOUNDCARD)) == -1) {
      throw_exception<mixer_error>("Failed to attach hardware mixer control", err);
    }
    if ((err = snd_mixer_selem_register(m_hardwaremixer, nullptr, nullptr)) == -1) {
      throw_exception<mixer_error>("Failed to register simple mixer element", err);
    }
    if ((err = snd_mixer_load(m_hardwaremixer)) == -1) {
      throw_exception<mixer_error>("Failed to load mixer", err);
    }

    snd_mixer_selem_id_set_index(m_mixerid, 0);
    snd_mixer_selem_id_set_name(m_mixerid, m_name.c_str());

    if ((m_mixerelement = snd_mixer_find_selem(m_hardwaremixer, m_mixerid)) == nullptr) {
      throw mixer_error("Cannot find simple element");
    }
  }

  /**
   * Deconstruct mixer
   */
  mixer::~mixer() {
    std::lock_guard<std::mutex> guard(m_lock);
    if (m_mixerid != nullptr) {
      snd_mixer_selem_id_free(m_mixerid);
    }
    if (m_mixerelement != nullptr) {
      snd_mixer_elem_remove(m_mixerelement);
    }
    if (m_hardwaremixer != nullptr) {
      snd_mixer_detach(m_hardwaremixer, ALSA_SOUNDCARD);
      snd_mixer_close(m_hardwaremixer);
    }
  }

  /**
   * Get mixer name
   */
  string mixer::get_name() {
    return m_name;
  }

  /**
   * Wait for events
   */
  bool mixer::wait(int timeout) {
    assert(m_hardwaremixer);

    if (!m_lock.try_lock()) {
      return false;
    }

    std::unique_lock<std::mutex> guard(m_lock, std::adopt_lock);

    int err = 0;

    if ((err = snd_mixer_wait(m_hardwaremixer, timeout)) == -1) {
      throw_exception<mixer_error>("Failed to wait for events", err);
    }

    guard.unlock();

    return process_events() > 0;
  }

  /**
   * Process queued mixer events
   */
  int mixer::process_events() {
    if (!m_lock.try_lock()) {
      return false;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    int num_events = snd_mixer_handle_events(m_hardwaremixer);
    if (num_events == -1) {
      throw_exception<mixer_error>("Failed to process pending events", num_events);
    }

    return num_events;
  }

  /**
   * Get volume in percentage
   */
  int mixer::get_volume() {
    if (!m_lock.try_lock()) {
      return 0;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    long chan_n = 0, vol_total = 0, vol, vol_min, vol_max;

    snd_mixer_selem_get_playback_volume_range(m_mixerelement, &vol_min, &vol_max);

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        snd_mixer_selem_get_playback_volume(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i), &vol);
        vol_total += vol;
        chan_n++;
      }
    }

    return math_util::percentage(vol_total / chan_n, vol_min, vol_max);
  }

  /**
   * Get normalized volume in percentage
   */
  int mixer::get_normalized_volume() {
    if (!m_lock.try_lock()) {
      return 0;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    long chan_n = 0, vol_total = 0, vol, vol_min, vol_max;
    double normalized, min_norm;

    snd_mixer_selem_get_playback_dB_range(m_mixerelement, &vol_min, &vol_max);

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        snd_mixer_selem_get_playback_dB(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i), &vol);
        vol_total += vol;
        chan_n++;
      }
    }

    if (vol_max - vol_min <= MAX_LINEAR_DB_SCALE * 100) {
      return math_util::percentage(vol_total / chan_n, vol_min, vol_max);
    }

    normalized = pow10((vol_total / chan_n - vol_max) / 6000.0);
    if (vol_min != SND_CTL_TLV_DB_GAIN_MUTE) {
      min_norm = pow10((vol_min - vol_max) / 6000.0);
      normalized = (normalized - min_norm) / (1 - min_norm);
    }

    return 100.0f * normalized + 0.5f;
  }

  /**
   * Set volume to given percentage
   */
  void mixer::set_volume(float percentage) {
    if (is_muted()) {
      return;
    }

    if (!m_lock.try_lock()) {
      return;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    long vol_min, vol_max;
    snd_mixer_selem_get_playback_volume_range(m_mixerelement, &vol_min, &vol_max);
    snd_mixer_selem_set_playback_volume_all(
        m_mixerelement, math_util::percentage_to_value<int>(percentage, vol_min, vol_max));
  }

  /**
   * Set normalized volume to given percentage
   */
  void mixer::set_normalized_volume(float percentage) {
    if (is_muted()) {
      return;
    }

    if (!m_lock.try_lock()) {
      return;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    long vol_min, vol_max;
    double min_norm;
    percentage = percentage / 100.0f;

    snd_mixer_selem_get_playback_dB_range(m_mixerelement, &vol_min, &vol_max);

    if (vol_max - vol_min <= MAX_LINEAR_DB_SCALE * 100) {
      snd_mixer_selem_set_playback_dB_all(m_mixerelement, lrint(percentage * (vol_max - vol_min)) + vol_min, 0);
      return;
    }

    if (vol_min != SND_CTL_TLV_DB_GAIN_MUTE) {
      min_norm = pow10((vol_min - vol_max) / 6000.0);
      percentage = percentage * (1 - min_norm) + min_norm;
    }

    snd_mixer_selem_set_playback_dB_all(m_mixerelement, lrint(6000.0 * log10(percentage)) + vol_max, 0);
  }

  /**
   * Set mute state
   */
  void mixer::set_mute(bool mode) {
    if (!m_lock.try_lock()) {
      return;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    snd_mixer_selem_set_playback_switch_all(m_mixerelement, mode);
  }

  /**
   * Toggle mute state
   */
  void mixer::toggle_mute() {
    if (!m_lock.try_lock()) {
      return;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    int state;

    snd_mixer_selem_get_playback_switch(m_mixerelement, SND_MIXER_SCHN_MONO, &state);
    snd_mixer_selem_set_playback_switch_all(m_mixerelement, !state);
  }

  /**
   * Get current mute state
   */
  bool mixer::is_muted() {
    if (!m_lock.try_lock()) {
      return false;
    }

    std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

    int state = 0;

    for (int i = 0; i <= SND_MIXER_SCHN_LAST; i++) {
      if (snd_mixer_selem_has_playback_channel(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i))) {
        int state_ = 0;
        snd_mixer_selem_get_playback_switch(m_mixerelement, static_cast<snd_mixer_selem_channel_id_t>(i), &state_);
        state = state || state_;
      }
    }
    return !state;
  }
}

POLYBAR_NS_END

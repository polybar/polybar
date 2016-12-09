#include "adapters/alsa.hpp"
#include "utils/math.hpp"

POLYBAR_NS

// class : alsa_ctl_interface {{{

alsa_ctl_interface::alsa_ctl_interface(int numid) : m_numid(numid) {
  snd_ctl_elem_info_malloc(&m_info);

  if (m_info == nullptr) {
    throw alsa_ctl_interface_error("Failed to allocate alsa_ctl info");
  }

  snd_ctl_elem_value_malloc(&m_value);

  if (m_value == nullptr) {
    throw alsa_ctl_interface_error("Failed to allocate alsa_ctl value");
  }

  snd_ctl_elem_id_malloc(&m_id);

  if (m_id == nullptr) {
    throw alsa_ctl_interface_error("Failed to allocate alsa_ctl id");
  }

  snd_ctl_elem_id_set_numid(m_id, m_numid);
  snd_ctl_elem_info_set_id(m_info, m_id);

  int err = 0;

  if ((err = snd_ctl_open(&m_ctl, ALSA_SOUNDCARD, SND_CTL_NONBLOCK | SND_CTL_READONLY)) == -1) {
    throw_exception<alsa_ctl_interface_error>("Could not open control '" + string{ALSA_SOUNDCARD} + "'", err);
  }

  snd_config_update_free_global();

  if ((err = snd_ctl_elem_info(m_ctl, m_info)) < 0) {
    throw_exception<alsa_ctl_interface_error>("Could not get control datal", err);
  }

  snd_ctl_elem_info_get_id(m_info, m_id);

  if ((err = snd_hctl_open(&m_hctl, ALSA_SOUNDCARD, 0)) == -1) {
    throw_exception<alsa_ctl_interface_error>("Failed to open hctl", err);
  }

  snd_config_update_free_global();

  if (m_hctl == nullptr || (err = snd_hctl_load(m_hctl)) < 0) {
    throw_exception<alsa_ctl_interface_error>("Failed to load hctl", err);
  }

  if ((m_elem = snd_hctl_find_elem(m_hctl, m_id)) == nullptr) {
    throw alsa_ctl_interface_error("Could not find control with id " + to_string(snd_ctl_elem_id_get_numid(m_id)));
  }

  if ((err = snd_ctl_subscribe_events(m_ctl, 1)) < 0) {
    throw alsa_ctl_interface_error("Could not subscribe to events: " + to_string(snd_ctl_elem_id_get_numid(m_id)));
  }
}

alsa_ctl_interface::~alsa_ctl_interface() {
  std::lock_guard<std::mutex> guard(m_lock);
  if (m_info != nullptr) {
    snd_ctl_elem_info_free(m_info);
  }
  if (m_value != nullptr) {
    snd_ctl_elem_value_free(m_value);
  }
  if (m_id != nullptr) {
    snd_ctl_elem_id_free(m_id);
  }
  if (m_ctl != nullptr) {
    snd_ctl_close(m_ctl);
  }
  if (m_hctl != nullptr) {
    snd_hctl_close(m_hctl);
  }
}

int alsa_ctl_interface::get_numid() {
  return m_numid;
}

bool alsa_ctl_interface::wait(int timeout) {
  assert(m_ctl);

  if (!m_lock.try_lock()) {
    return false;
  }

  std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

  int err = 0;

  if ((err = snd_ctl_wait(m_ctl, timeout)) == -1) {
    throw_exception<alsa_ctl_interface_error>("Failed to wait for events", err);
  }

  snd_ctl_event_t* event;
  snd_ctl_event_alloca(&event);

  if ((err = snd_ctl_read(m_ctl, event)) == -1) {
    return false;
  }

  if (snd_ctl_event_get_type(event) == SND_CTL_EVENT_ELEM) {
    return snd_ctl_event_elem_get_mask(event) & SND_CTL_EVENT_MASK_VALUE;
  }

  return false;
}

bool alsa_ctl_interface::test_device_plugged() {
  assert(m_elem);
  assert(m_value);

  if (!m_lock.try_lock()) {
    return false;
  }

  std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

  int err = 0;
  if ((err = snd_hctl_elem_read(m_elem, m_value)) < 0) {
    throw_exception<alsa_ctl_interface_error>("Could not read control value", err);
  }
  return snd_ctl_elem_value_get_boolean(m_value, 0);
}

void alsa_ctl_interface::process_events() {
  wait(0);
}

// }}}
// class : alsa_mixer {{{

alsa_mixer::alsa_mixer(string mixer_control_name) : m_name(move(mixer_control_name)) {
  if (m_name.empty()) {
    throw alsa_mixer_error("Invalid control name");
  }

  snd_mixer_selem_id_malloc(&m_mixerid);

  if (m_mixerid == nullptr) {
    throw alsa_mixer_error("Failed to allocate mixer id");
  }

  int err = 0;
  if ((err = snd_mixer_open(&m_hardwaremixer, 1)) == -1) {
    throw_exception<alsa_mixer_error>("Failed to open hardware mixer", err);
  }

  snd_config_update_free_global();

  if ((err = snd_mixer_attach(m_hardwaremixer, ALSA_SOUNDCARD)) == -1) {
    throw_exception<alsa_mixer_error>("Failed to attach hardware mixer control", err);
  }
  if ((err = snd_mixer_selem_register(m_hardwaremixer, nullptr, nullptr)) == -1) {
    throw_exception<alsa_mixer_error>("Failed to register simple mixer element", err);
  }
  if ((err = snd_mixer_load(m_hardwaremixer)) == -1) {
    throw_exception<alsa_mixer_error>("Failed to load mixer", err);
  }

  snd_mixer_selem_id_set_index(m_mixerid, 0);
  snd_mixer_selem_id_set_name(m_mixerid, m_name.c_str());

  if ((m_mixerelement = snd_mixer_find_selem(m_hardwaremixer, m_mixerid)) == nullptr) {
    throw alsa_mixer_error("Cannot find simple element");
  }

  // log_trace("Successfully initialized mixer: "+ string{m_name});
}

alsa_mixer::~alsa_mixer() {
  std::lock_guard<std::mutex> guard(m_lock);
  if (m_mixerid != nullptr) {
    snd_mixer_selem_id_free(m_mixerid);
  }
  if (m_mixerelement != nullptr) {
    snd_mixer_elem_free(m_mixerelement);
  }
  if (m_hardwaremixer != nullptr) {
    snd_mixer_detach(m_hardwaremixer, ALSA_SOUNDCARD);
    snd_mixer_close(m_hardwaremixer);
  }
}

string alsa_mixer::get_name() {
  return m_name;
}

bool alsa_mixer::wait(int timeout) {
  assert(m_hardwaremixer);

  if (!m_lock.try_lock()) {
    return false;
  }

  std::unique_lock<std::mutex> guard(m_lock, std::adopt_lock);

  int err = 0;

  if ((err = snd_mixer_wait(m_hardwaremixer, timeout)) == -1) {
    throw_exception<alsa_mixer_error>("Failed to wait for events", err);
  }

  guard.unlock();

  return process_events() > 0;
}

int alsa_mixer::process_events() {
  if (!m_lock.try_lock()) {
    return false;
  }

  std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

  int num_events = snd_mixer_handle_events(m_hardwaremixer);
  if (num_events < 0) {
    throw_exception<alsa_mixer_error>("Failed to process pending events", num_events);
  }

  return num_events;
}

int alsa_mixer::get_volume() {
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

int alsa_mixer::get_normalized_volume() {
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

void alsa_mixer::set_volume(float percentage) {
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

void alsa_mixer::set_normalized_volume(float percentage) {
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

void alsa_mixer::set_mute(bool mode) {
  if (!m_lock.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

  snd_mixer_selem_set_playback_switch_all(m_mixerelement, mode);
}

void alsa_mixer::toggle_mute() {
  if (!m_lock.try_lock()) {
    return;
  }

  std::lock_guard<std::mutex> guard(m_lock, std::adopt_lock);

  int state;

  snd_mixer_selem_get_playback_switch(m_mixerelement, SND_MIXER_SCHN_MONO, &state);
  snd_mixer_selem_set_playback_switch_all(m_mixerelement, !state);
}

bool alsa_mixer::is_muted() {
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

// }}}

POLYBAR_NS_END

#include "config.hpp"
#include "interfaces/alsa.hpp"
#include "services/logger.hpp"
#include "utils/config.hpp"
#include "utils/memory.hpp"
#include "utils/proc.hpp"
#include "utils/macros.hpp"

namespace alsa
{
  // ControlInterface {{{

  ControlInterface::ControlInterface(int numid)
  {
    int err;

    snd_ctl_elem_info_alloca(&this->info);
    snd_ctl_elem_value_alloca(&this->value);
    snd_ctl_elem_id_alloca(&this->id);

    snd_ctl_elem_id_set_numid(this->id, numid);
    snd_ctl_elem_info_set_id(this->info, this->id);

    if ((err = snd_ctl_open(&this->ctl, ALSA_SOUNDCARD, SND_CTL_NONBLOCK | SND_CTL_READONLY)) < 0)
      throw ControlInterfaceError(err, "Could not open control \""+ ToStr(ALSA_SOUNDCARD) +"\": "+ StrSndErr(err));

    if ((err = snd_ctl_elem_info(this->ctl, this->info)) < 0)
      throw ControlInterfaceError(err, "Could not get control data: "+ StrSndErr(err));

    snd_ctl_elem_info_get_id(this->info, this->id);

    if ((err = snd_hctl_open(&this->hctl, ALSA_SOUNDCARD, 0)) < 0)
      throw ControlInterfaceError(err, StrSndErr(err));
    if ((err = snd_hctl_load(this->hctl)) < 0)
      throw ControlInterfaceError(err, StrSndErr(err));
    if ((elem = snd_hctl_find_elem(this->hctl, this->id)) == nullptr)
      throw ControlInterfaceError(err, "Could not find control with id "+ IntToStr(snd_ctl_elem_id_get_numid(this->id)));

    if ((err = snd_ctl_subscribe_events(this->ctl, 1)) < 0)
      throw ControlInterfaceError(err, "Could not subscribe to events: "+ IntToStr(snd_ctl_elem_id_get_numid(this->id)));

    log_trace("Successfully initialized control interface with ID: "+ IntToStr(numid));
  }

  ControlInterface::~ControlInterface()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    snd_ctl_close(this->ctl);
    snd_hctl_close(this->hctl);
  }

  bool ControlInterface::wait(int timeout)
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    int err;

    if ((err = snd_ctl_wait(this->ctl, timeout)) < 0)
      throw ControlInterfaceError(err, "Failed to wait for events: "+ StrSndErr(err));

    snd_ctl_event_t *event;
    snd_ctl_event_alloca(&event);

    if ((err = snd_ctl_read(this->ctl, event)) < 0)
      return false;
    if (snd_ctl_event_get_type(event) != SND_CTL_EVENT_ELEM)
      return false;

    auto mask = snd_ctl_event_elem_get_mask(event);

    return mask & SND_CTL_EVENT_MASK_VALUE;
  }

  bool ControlInterface::test_device_plugged()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    int err;

    if ((err = snd_hctl_elem_read(this->elem, this->value)) < 0)
      throw ControlInterfaceError(err, "Could not read control value: "+ StrSndErr(err));

    return snd_ctl_elem_value_get_boolean(this->value, 0);
  }

  // }}}
  // Mixer {{{

  Mixer::Mixer(std::string mixer_control_name)
  {
    snd_mixer_selem_id_t *mixer_id;

    snd_mixer_selem_id_alloca(&mixer_id);

    if (snd_mixer_open(&this->hardware_mixer, 1) < 0)
      throw MixerError("Failed to open hardware mixer");
    if (snd_mixer_attach(this->hardware_mixer, ALSA_SOUNDCARD) < 0)
      throw MixerError("Failed to attach hardware mixer control");
    if (snd_mixer_selem_register(this->hardware_mixer, nullptr, nullptr) < 0)
      throw MixerError("Failed to register simple mixer element");
    if (snd_mixer_load(this->hardware_mixer) < 0)
      throw MixerError("Failed to load mixer");

    snd_mixer_selem_id_set_index(mixer_id, 0);
    snd_mixer_selem_id_set_name(mixer_id, mixer_control_name.c_str());

    if ((this->mixer_element = snd_mixer_find_selem(this->hardware_mixer, mixer_id)) == nullptr)
      throw MixerError("Cannot find simple element");

    log_trace("Successfully initialized mixer: "+ mixer_control_name);
  }

  Mixer::~Mixer()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    snd_mixer_elem_remove(this->mixer_element);
    snd_mixer_detach(this->hardware_mixer, ALSA_SOUNDCARD);
    snd_mixer_close(this->hardware_mixer);
  }

  int Mixer::process_events()
  {
    int num_events = snd_mixer_handle_events(this->hardware_mixer);

    if (num_events < 0)
      throw MixerError("Failed to process pending events: "+ StrSndErr(num_events));

    return num_events;
  }

  bool Mixer::wait(int timeout)
  {
    assert(this->hardware_mixer);

    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    int err = snd_mixer_wait(this->hardware_mixer, timeout);

    if (err < 0)
      throw MixerError("Failed to wait for events: "+ StrSndErr(err));

    return this->process_events() > 0;
  }

  int Mixer::get_volume()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    long chan_n = 0, vol_total = 0, vol, vol_min, vol_max;

    snd_mixer_selem_get_playback_volume_range(this->mixer_element, &vol_min, &vol_max);

    repeat(SND_MIXER_SCHN_LAST)
    {
      if (snd_mixer_selem_has_playback_channel(this->mixer_element, (snd_mixer_selem_channel_id_t) repeat_i)) {
        snd_mixer_selem_get_playback_volume(this->mixer_element, (snd_mixer_selem_channel_id_t) repeat_i, &vol);
        vol_total += vol;
        chan_n++;
      }
    }

    return (int) 100.0f * (vol_total / chan_n) / vol_max + 0.5f;
  }

  void Mixer::set_volume(float percentage)
  {
    if (this->is_muted())
      return;

    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    long vol_min, vol_max;

    snd_mixer_selem_get_playback_volume_range(this->mixer_element, &vol_min, &vol_max);
    snd_mixer_selem_set_playback_volume_all(this->mixer_element, vol_max * percentage / 100);
  }

  void Mixer::set_mute(bool mode)
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    snd_mixer_selem_set_playback_switch_all(this->mixer_element, mode);
  }

  void Mixer::toggle_mute()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    int state;
    snd_mixer_selem_get_playback_switch(this->mixer_element, SND_MIXER_SCHN_FRONT_LEFT, &state);
    snd_mixer_selem_set_playback_switch_all(this->mixer_element, !state);
  }

  bool Mixer::is_muted()
  {
    std::lock_guard<concurrency::SpinLock> lck(this->lock);

    int state = 0;
    repeat(SND_MIXER_SCHN_LAST)
    {
      if (snd_mixer_selem_has_playback_channel(this->mixer_element, (snd_mixer_selem_channel_id_t) repeat_i)) {
        snd_mixer_selem_get_playback_switch(this->mixer_element, SND_MIXER_SCHN_FRONT_LEFT, &state);
        if (state == 0)
          return true;
      }
    }

    return false;
  }

  // }}}
}

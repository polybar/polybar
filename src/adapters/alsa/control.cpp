#include "adapters/alsa/control.hpp"
#include "adapters/alsa/generic.hpp"

POLYBAR_NS

namespace alsa {
  /**
   * Construct control object
   */
  control::control(int numid) : m_numid(numid) {
    int err{0};

    if ((err = snd_ctl_open(&m_ctl, ALSA_SOUNDCARD, SND_CTL_NONBLOCK | SND_CTL_READONLY)) == -1) {
      throw_exception<control_error>("Could not open control '" + string{ALSA_SOUNDCARD} + "'", err);
    }

    snd_config_update_free_global();

    if ((err = snd_hctl_open_ctl(&m_hctl, m_ctl)) == -1) {
      snd_ctl_close(m_ctl);
      throw_exception<control_error>("Failed to open hctl", err);
    }

    snd_config_update_free_global();

    if ((err = snd_hctl_load(m_hctl)) == -1) {
      throw_exception<control_error>("Failed to load hctl", err);
    }

    snd_ctl_elem_id_t* m_id{nullptr};
    snd_ctl_elem_id_alloca(&m_id);
    snd_ctl_elem_id_set_numid(m_id, m_numid);

    snd_ctl_elem_info_t* m_info{nullptr};
    snd_ctl_elem_info_alloca(&m_info);
    snd_ctl_elem_info_set_id(m_info, m_id);

    if ((err = snd_ctl_elem_info(m_ctl, m_info)) == -1) {
      throw_exception<control_error>("Could not get control data", err);
    }

    snd_ctl_elem_info_get_id(m_info, m_id);

    if ((m_elem = snd_hctl_find_elem(m_hctl, m_id)) == nullptr) {
      throw control_error("Could not find control with id " + to_string(snd_ctl_elem_id_get_numid(m_id)));
    }

    if ((err = snd_ctl_subscribe_events(m_ctl, 1)) == -1) {
      throw control_error("Could not subscribe to events: " + to_string(snd_ctl_elem_id_get_numid(m_id)));
    }
  }

  /**
   * Deconstruct control object
   */
  control::~control() {
    if (m_hctl != nullptr) {
      snd_hctl_close(m_hctl);
    }
  }

  /**
   * Get the id number
   */
  int control::get_numid() {
    return m_numid;
  }

  /**
   * Wait for events
   */
  bool control::wait(int timeout) {
    assert(m_ctl);

    int err{0};

    if ((err = snd_ctl_wait(m_ctl, timeout)) == -1) {
      throw_exception<control_error>("Failed to wait for events", err);
    }

    snd_ctl_event_t* event{nullptr};
    snd_ctl_event_alloca(&event);

    if ((err = snd_ctl_read(m_ctl, event)) == -1) {
      return false;
    }

    if (snd_ctl_event_get_type(event) == SND_CTL_EVENT_ELEM) {
      return snd_ctl_event_elem_get_mask(event) & SND_CTL_EVENT_MASK_VALUE;
    }

    return false;
  }

  /**
   * Check if the interface is in use
   */
  bool control::test_device_plugged() {
    assert(m_elem);

    snd_ctl_elem_value_t* m_value{nullptr};
    snd_ctl_elem_value_alloca(&m_value);

    int err{0};

    if ((err = snd_hctl_elem_read(m_elem, m_value)) == -1) {
      throw_exception<control_error>("Could not read control value", err);
    }

    return snd_ctl_elem_value_get_boolean(m_value, 0);
  }

  /**
   * Process queued events
   */
  void control::process_events() {
    wait(0);
  }
}

POLYBAR_NS_END

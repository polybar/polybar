#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#include "errors.hpp"
#include "adapters/oss.hpp"

POLYBAR_NS

namespace oss {
  string sound_device_names[] = SOUND_DEVICE_NAMES;

  void throw_exception(string&& message = "", int error_code = 0) {
    if (error_code != 0)
      message += ": " + string{strerror(error_code)};
    throw oss_exception(message.c_str());
  }

  /**
   * Construct mixer object
   */
  mixer::mixer(string&& mixer_device_path, string&& mixer_channel_name)
      : m_device_path(std::forward<string>(mixer_device_path)) {
    if ((m_fd = open(m_device_path.c_str(), O_RDWR)) == -1) {
      throw_exception("Failed to open mixer", errno);
    }

    for (int i = 0; i < SOUND_MIXER_NONE; i++) {
      if (strcasecmp(mixer_channel_name.c_str(), sound_device_names[i].c_str()) == 0) {
        m_channel = i;
        break;
      }
    }
    if (m_channel == SOUND_MIXER_NONE) {
      throw_exception("Mixer name is not valid");
    }

    int mask;
    if (ioctl(m_fd, MIXER_READ(m_channel), &mask) == -1) {
      throw_exception("Mixer is not available on the soundcard");
    }

    if (ioctl(m_fd, SOUND_MIXER_READ_STEREODEVS, &m_stereodevs) == -1) {
      throw_exception("Cannot determine stereo/mono channels", errno);
    }
  }

  /**
   * Deconstruct mixer
   */
  mixer::~mixer() {
    if (m_fd != -1) {
      close(m_fd);
    }
  }

  /**
   * Get mixer name
   */
  const string& mixer::get_name() {
    return sound_device_names[m_channel];
  }

  /**
   * Get the name of the soundcard that is associated with the mixer
   */
  const string& mixer::get_sound_card() {
#if defined(SOUND_MIXER_INFO)
    mixer_info mi;
    if (ioctl(m_fd, SOUND_MIXER_INFO, &mi) == -1) {
      throw_exception("Failed to get mixer info", errno);
    }
    std::string *name = new std::string{mi.name};
    return *name;
#else
    return m_device_path;
#endif
  }

  /**
   * Wait for events
   */
  bool mixer::wait(int timeout) {
    struct timeval to;
    to.tv_sec = timeout / 1000;
    to.tv_usec = (timeout-to.tv_sec) * 1000;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    if (select(m_fd+1, &fds, NULL, NULL, &to) == -1) {
      throw_exception("Failed to wait for events", errno);
    }
    return FD_ISSET(m_fd, &fds);
  }

  /**
   * Get volume in percentage
   */
  int mixer::get_volume() {
	uint32_t vol;
    if (ioctl(m_fd, MIXER_READ(m_channel), &vol) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
	return ((1<<m_channel) & m_stereodevs)
        ? ((vol & 0x7f) + ((vol & 0x7f00)>>8)) / 2
        : (vol & 0x7f);
  }

  /**
   * Set volume to given percentage
   */
  void mixer::set_volume(float percentage) {
    uint32_t vol = ((1<<m_channel) & m_stereodevs)
        ? ((uint8_t)percentage) + ((uint8_t)percentage<<8)
        : ((uint8_t)percentage);
    if (ioctl(m_fd, MIXER_WRITE(m_channel), &vol) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
  }

  /**
   * Set mute state
   */
  void mixer::set_mute(bool mode) {
#ifdef SOUND_MIXER_READ_MUTE
    int mutemask;
    if (ioctl(m_fd, SOUND_MIXER_READ_MUTE, &mutemask) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
    if (mode) {
      mutemask &= ~(1 << m_channel);
    }
    else {
      mutemask |= (1 << m_channel);
    }
    if (ioctl(m_fd, SOUND_MIXER_WRITE_MUTE, &mutemask) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
#endif
  }

  /**
   * Toggle mute state
   */
  void mixer::toggle_mute() {
#ifdef SOUND_MIXER_READ_MUTE
    int mutemask;
    if (ioctl(m_fd, SOUND_MIXER_READ_MUTE, &mutemask) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
    mutemask ^= (1 << m_channel);
    if (ioctl(m_fd, SOUND_MIXER_WRITE_MUTE, &mutemask) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
#endif
  }

  /**
   * Get current mute state
   */
  bool mixer::is_muted() {
#ifdef SOUND_MIXER_READ_MUTE
    int mutemask;
    if (ioctl(m_fd, SOUND_MIXER_READ_MUTE, &mutemask) == -1) {
      throw_exception("Undefined mixer channel was accessed");
    }
    return (mutemask & (1 << m_channel));
#else
    return false;
#endif
  }
}

POLYBAR_NS_END

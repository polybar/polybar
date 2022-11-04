#pragma once

#include <mutex>

#include "common.hpp"
#include "settings.hpp"

// fwd
struct _snd_mixer;
struct _snd_mixer_elem;
struct _snd_mixer_selem_id;
typedef struct _snd_mixer snd_mixer_t;
typedef struct _snd_mixer_elem snd_mixer_elem_t;
typedef struct _snd_mixer_selem_id snd_mixer_selem_id_t;

POLYBAR_NS

namespace alsa {
  class mixer {
   public:
    explicit mixer(string&& mixer_selem_name, string&& soundcard_name);
    ~mixer();

    mixer(const mixer& o) = delete;
    mixer& operator=(const mixer& o) = delete;

    const string& get_name();
    const string& get_sound_card();

    bool wait(int timeout = -1);
    int process_events();

    int get_volume();
    int get_normalized_volume();
    void set_volume(float percentage);
    void set_normalized_volume(float percentage);
    void set_mute(bool mode);
    void toggle_mute();
    bool is_muted();

   private:
    snd_mixer_t* m_mixer{nullptr};
    snd_mixer_elem_t* m_elem{nullptr};

    string m_name;
    string s_name;
  };
}

POLYBAR_NS_END

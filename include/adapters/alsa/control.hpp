#pragma once

#include <mutex>

#include "common.hpp"
#include "settings.hpp"

// fwd
struct _snd_ctl;
struct _snd_hctl_elem;
struct _snd_hctl;
typedef struct _snd_ctl snd_ctl_t;
typedef struct _snd_hctl_elem snd_hctl_elem_t;
typedef struct _snd_hctl snd_hctl_t;

POLYBAR_NS

namespace alsa {
  class control {
   public:
    explicit control(int numid);
    ~control();

    control(const control& o) = delete;
    control& operator=(const control& o) = delete;

    int get_numid();
    bool wait(int timeout = -1);
    bool test_device_plugged();
    void process_events();

   private:
    int m_numid{0};

    snd_ctl_t* m_ctl{nullptr};
    snd_hctl_t* m_hctl{nullptr};
    snd_hctl_elem_t* m_elem{nullptr};
  };
}

POLYBAR_NS_END

#pragma once

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS
class logger;

DEFINE_ERROR(pulseaudio_error);

class pulseaudio {
 public:
  virtual ~pulseaudio(){};

  virtual const string& get_name() = 0;

  virtual bool wait() = 0;
  virtual int process_events() = 0;

  virtual int get_volume() = 0;
  virtual double get_decibels() = 0;
  virtual void set_volume(float percentage) = 0;
  virtual void inc_volume(int delta_perc) = 0;
  virtual void set_mute(bool mode) = 0;
  virtual void toggle_mute() = 0;
  virtual bool is_muted() = 0;
};

POLYBAR_NS_END

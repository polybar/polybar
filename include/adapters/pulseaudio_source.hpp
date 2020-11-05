#pragma once

#include <pulse/pulseaudio.h>

#include <queue>

#include "common.hpp"
#include "errors.hpp"
#include "pulseaudio.hpp"
#include "settings.hpp"
#include "utils/math.hpp"

// fwd
struct pa_context;
struct pa_threaded_mainloop;
struct pa_cvolume;
typedef struct pa_context pa_context;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;

POLYBAR_NS
class pulseaudio_source : public pulseaudio {
  // events to add to our queue
  enum class evtype { NEW = 0, CHANGE, REMOVE, SERVER };
  using queue = std::queue<evtype>;

 public:
  explicit pulseaudio_source(const logger& logger, string&& sink_name, bool m_max_volume);
  virtual ~pulseaudio_source();

  pulseaudio_source(const pulseaudio_source& o) = delete;
  pulseaudio_source& operator=(const pulseaudio_source& o) = delete;

  virtual const string& get_name();

  virtual bool wait();
  virtual int process_events();

  virtual int get_volume();
  virtual double get_decibels();
  virtual void set_volume(float percentage);
  virtual void inc_volume(int delta_perc);
  virtual void set_mute(bool mode);
  virtual void toggle_mute();
  virtual bool is_muted();

 private:
  void update_volume(pa_operation* o);
  static void check_mute_callback(pa_context* context, const pa_source_info* info, int eol, void* userdata);
  static void get_source_volume_callback(pa_context* context, const pa_source_info* info, int is_last, void* userdata);
  static void subscribe_callback(pa_context* context, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
  static void simple_callback(pa_context* context, int success, void* userdata);
  static void source_info_callback(pa_context* context, const pa_source_info* info, int eol, void* userdata);
  static void context_state_callback(pa_context* context, void* userdata);

  inline void wait_loop(pa_operation* op, pa_threaded_mainloop* loop);

  const logger& m_log;

  // used for temporary callback results
  int success{0};
  pa_cvolume cv;
  bool muted{false};
  // default sink name
  static constexpr auto DEFAULT_SOURCE{"@DEFAULT_SOURCE@"};

  pa_context* m_context{nullptr};
  pa_threaded_mainloop* m_mainloop{nullptr};

  queue m_events;

  // specified source name
  string spec_s_name;
  string s_name;
  uint32_t m_index{0};

  pa_volume_t m_max_volume{PA_VOLUME_UI_MAX};
};

POLYBAR_NS_END

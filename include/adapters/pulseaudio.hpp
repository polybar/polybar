#pragma once

#include <pulse/pulseaudio.h>

#include <atomic>
#include <queue>

#include "common.hpp"
#include "errors.hpp"
#include "settings.hpp"
#include "utils/math.hpp"
// fwd
struct pa_context;
struct pa_threaded_mainloop;
struct pa_cvolume;
typedef struct pa_context pa_context;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;

POLYBAR_NS
class logger;

DEFINE_ERROR(pulseaudio_error);

class pulseaudio {
  // events to add to our queue
  enum class evtype { NEW = 0, CHANGE, REMOVE, SERVER };
  using queue = std::queue<evtype>;

 public:
  explicit pulseaudio(const logger& logger, string&& sink_name, bool m_max_volume);
  ~pulseaudio();

  pulseaudio(const pulseaudio& o) = delete;
  pulseaudio& operator=(const pulseaudio& o) = delete;

  const string& get_name();

  bool wait();
  int process_events();

  int get_volume();
  double get_decibels();
  void set_volume(float percentage);
  void inc_volume(int delta_perc);
  void set_mute(bool mode);
  void toggle_mute();
  bool is_muted();

 private:
  void update_volume(pa_operation* o);
  static void check_mute_callback(pa_context* context, const pa_sink_info* info, int eol, void* userdata);
  static void get_sink_volume_callback(pa_context* context, const pa_sink_info* info, int is_last, void* userdata);
  static void subscribe_callback(pa_context* context, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
  static void simple_callback(pa_context* context, int success, void* userdata);
  static void sink_info_callback(pa_context* context, const pa_sink_info* info, int eol, void* userdata);
  static void context_state_callback(pa_context* context, void* userdata);

  inline void wait_loop(pa_operation* op, pa_threaded_mainloop* loop);

  const logger& m_log;

  /**
   * Has context_state_callback signalled the mainloop during connection.
   */
  std::atomic_bool m_state_callback_signal{false};

  // used for temporary callback results
  int success{0};
  pa_cvolume cv{};
  bool muted{false};
  // default sink name
  static constexpr auto DEFAULT_SINK = "@DEFAULT_SINK@";

  pa_context* m_context{nullptr};
  pa_threaded_mainloop* m_mainloop{nullptr};

  queue m_events;

  // specified sink name
  string spec_s_name;
  string s_name;
  uint32_t m_index{0};

  pa_volume_t m_max_volume{PA_VOLUME_UI_MAX};
};

POLYBAR_NS_END

#pragma once

#include <pulse/pulseaudio.h>
#include <queue>

#include "common.hpp"
#include "settings.hpp"
#include "errors.hpp"

#include "utils/math.hpp"
// fwd
struct pa_context;
struct pa_threaded_mainloop;
struct pa_cvolume;
typedef struct pa_context pa_context;
typedef struct pa_threaded_mainloop pa_threaded_mainloop;

POLYBAR_NS

DEFINE_ERROR(pulseaudio_error);

class pulseaudio {
  // events to add to our queue
  enum class evtype { NEW = 0, CHANGE, REMOVE };
  using queue = std::queue<evtype>;

  public:
    explicit pulseaudio(string&& sink_name);
    ~pulseaudio();

    pulseaudio(const pulseaudio& o) = delete;
    pulseaudio& operator=(const pulseaudio& o) = delete;

    const string& get_name();

    bool wait(int timeout = -1);
    int process_events();

    int get_volume();
    void set_volume(float percentage);
    void inc_volume(int delta_perc);
    void set_mute(bool mode);
    void toggle_mute();
    bool is_muted();

  private:
    static void check_mute_callback(pa_context *context, const pa_sink_info *info, int eol, void *userdata);
    static void get_sink_volume_callback(pa_context *context, const pa_sink_info *info, int is_last, void *userdata);
    static void subscribe_callback(pa_context* context, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
    static void simple_callback(pa_context *context, int success, void *userdata);
    static void get_default_sink_callback(pa_context *context, const pa_server_info *info, void *userdata);
    static void sink_info_callback(pa_context *context, const pa_sink_info *info, int eol, void *userdata);
    static void context_state_callback(pa_context *context, void *userdata);

    inline void wait_loop(pa_operation *op, pa_threaded_mainloop *loop);

    // used for temporary callback results
    int success;
    pa_cvolume cv;
    bool muted;
    bool exists;

    pa_context* m_context{nullptr};
    pa_threaded_mainloop* m_mainloop{nullptr};

    queue m_events;

    // specified sink name
    string spec_s_name;
    // default sink name
    string def_s_name;
    uint32_t m_index{0};
};

POLYBAR_NS_END

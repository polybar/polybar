#include "adapters/pulseaudio.hpp"

#include "components/logger.hpp"

POLYBAR_NS

/**
 * Construct pulseaudio object
 */
pulseaudio::pulseaudio(const logger& logger, string&& sink_name, bool max_volume)
    : m_log(logger), spec_s_name(sink_name) {
  m_mainloop = pa_threaded_mainloop_new();
  if (!m_mainloop) {
    throw pulseaudio_error("Could not create pulseaudio threaded mainloop.");
  }
  pa_threaded_mainloop_lock(m_mainloop);

  m_context = pa_context_new(pa_threaded_mainloop_get_api(m_mainloop), "polybar");
  if (!m_context) {
    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_free(m_mainloop);
    throw pulseaudio_error("Could not create pulseaudio context.");
  }

  pa_context_set_state_callback(m_context, context_state_callback, this);

  m_state_callback_signal = false;

  if (pa_context_connect(m_context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
    pa_context_disconnect(m_context);
    pa_context_unref(m_context);
    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_free(m_mainloop);
    throw pulseaudio_error("Could not connect pulseaudio context.");
  }

  if (pa_threaded_mainloop_start(m_mainloop) < 0) {
    pa_context_disconnect(m_context);
    pa_context_unref(m_context);
    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_free(m_mainloop);
    throw pulseaudio_error("Could not start pulseaudio mainloop.");
  }

  m_log.trace("pulseaudio: started mainloop");

  /*
   * Only wait for signal from the context state callback, if it has not
   * already signalled the mainloop since pa_context_connect was called,
   * otherwise, we would wait forever.
   *
   * The while loop ensures spurious wakeups are handled.
   */
  while (!m_state_callback_signal) {
    pa_threaded_mainloop_wait(m_mainloop);
  }

  if (pa_context_get_state(m_context) != PA_CONTEXT_READY) {
    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_stop(m_mainloop);
    pa_context_disconnect(m_context);
    pa_context_unref(m_context);
    pa_threaded_mainloop_free(m_mainloop);
    throw pulseaudio_error("Could not connect to pulseaudio server.");
  }

  pa_operation* op{nullptr};
  if (!sink_name.empty()) {
    op = pa_context_get_sink_info_by_name(m_context, sink_name.c_str(), sink_info_callback, this);
    wait_loop(op, m_mainloop);
  }
  if (s_name.empty()) {
    // get the sink index
    op = pa_context_get_sink_info_by_name(m_context, DEFAULT_SINK, sink_info_callback, this);
    wait_loop(op, m_mainloop);
    m_log.notice("pulseaudio: using default sink %s", s_name);
  } else {
    m_log.trace("pulseaudio: using sink %s", s_name);
  }

  m_max_volume = max_volume ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;

  auto event_types = static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER);
  op = pa_context_subscribe(m_context, event_types, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success) {
    throw pulseaudio_error("Failed to subscribe to sink.");
  }
  pa_context_set_subscribe_callback(m_context, subscribe_callback, this);

  update_volume(op);

  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Deconstruct pulseaudio
 */
pulseaudio::~pulseaudio() {
  pa_threaded_mainloop_stop(m_mainloop);
  pa_context_disconnect(m_context);
  pa_context_unref(m_context);
  pa_threaded_mainloop_free(m_mainloop);
}

/**
 * Get sink name
 */
const string& pulseaudio::get_name() {
  return s_name;
}

/**
 * Wait for events
 */
bool pulseaudio::wait() {
  return m_events.size() > 0;
}

/**
 * Process queued pulseaudio events
 */
int pulseaudio::process_events() {
  int ret = m_events.size();
  pa_threaded_mainloop_lock(m_mainloop);
  pa_operation* o{nullptr};
  // clear the queue
  while (!m_events.empty()) {
    switch (m_events.front()) {
      // try to get specified sink
      case evtype::NEW:
        // redundant if already using specified sink
        if (!spec_s_name.empty()) {
          o = pa_context_get_sink_info_by_name(m_context, spec_s_name.c_str(), sink_info_callback, this);
          wait_loop(o, m_mainloop);
          break;
        }
        // FALLTHRU
      case evtype::SERVER:
        // don't fallthrough only if always using default sink
        if (!spec_s_name.empty()) {
          break;
        }
        // FALLTHRU
      // get default sink
      case evtype::REMOVE:
        o = pa_context_get_sink_info_by_name(m_context, DEFAULT_SINK, sink_info_callback, this);
        wait_loop(o, m_mainloop);
        if (spec_s_name != s_name) {
          m_log.notice("pulseaudio: using default sink %s", s_name);
        }
        break;
      default:
        break;
    }
    update_volume(o);
    m_events.pop();
  }
  pa_threaded_mainloop_unlock(m_mainloop);
  return ret;
}

/**
 * Get volume in percentage
 */
int pulseaudio::get_volume() {
  // alternatively, user pa_cvolume_avg_mask() to average selected channels
  return static_cast<int>(pa_cvolume_max(&cv) * 100.0f / PA_VOLUME_NORM + 0.5f);
}

/**
 * Get volume in decibels
 */
double pulseaudio::get_decibels() {
  return pa_sw_volume_to_dB(pa_cvolume_max(&cv));
}

/**
 * Set volume to given percentage
 */
void pulseaudio::set_volume(float percentage) {
  pa_threaded_mainloop_lock(m_mainloop);
  pa_volume_t vol = math_util::percentage_to_value<pa_volume_t>(percentage, PA_VOLUME_MUTED, PA_VOLUME_NORM);
  pa_cvolume_scale(&cv, vol);
  pa_operation* op = pa_context_set_sink_volume_by_index(m_context, m_index, &cv, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to set sink volume.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Increment or decrement volume by given percentage (prevents accumulation of rounding errors from get_volume)
 */
void pulseaudio::inc_volume(int delta_perc) {
  pa_threaded_mainloop_lock(m_mainloop);
  pa_volume_t vol = math_util::percentage_to_value<pa_volume_t>(abs(delta_perc), PA_VOLUME_NORM);
  if (delta_perc > 0) {
    pa_volume_t current = pa_cvolume_max(&cv);
    if (current + vol <= m_max_volume) {
      pa_cvolume_inc(&cv, vol);
    } else if (current < m_max_volume) {
      // avoid rounding errors and set to m_max_volume directly
      pa_cvolume_scale(&cv, m_max_volume);
    } else {
      m_log.notice("pulseaudio: maximum volume reached");
    }
  } else
    pa_cvolume_dec(&cv, vol);
  pa_operation* op = pa_context_set_sink_volume_by_index(m_context, m_index, &cv, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to set sink volume.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Set mute state
 */
void pulseaudio::set_mute(bool mode) {
  pa_threaded_mainloop_lock(m_mainloop);
  pa_operation* op = pa_context_set_sink_mute_by_index(m_context, m_index, mode, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to mute sink.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Toggle mute state
 */
void pulseaudio::toggle_mute() {
  set_mute(!is_muted());
}

/**
 * Get current mute state
 */
bool pulseaudio::is_muted() {
  return muted;
}

/**
 * Update local volume cache
 */
void pulseaudio::update_volume(pa_operation* o) {
  o = pa_context_get_sink_info_by_index(m_context, m_index, get_sink_volume_callback, this);
  wait_loop(o, m_mainloop);
}

/**
 * Callback when getting volume
 */
void pulseaudio::get_sink_volume_callback(pa_context*, const pa_sink_info* info, int, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  if (info) {
    This->cv = info->volume;
    This->muted = info->mute;
  }
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when subscribing to changes
 */
void pulseaudio::subscribe_callback(pa_context*, pa_subscription_event_type_t t, uint32_t idx, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SERVER:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_CHANGE:
          This->m_events.emplace(evtype::SERVER);
          break;
      }
      break;
    case PA_SUBSCRIPTION_EVENT_SINK:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_NEW:
          This->m_events.emplace(evtype::NEW);
          break;
        case PA_SUBSCRIPTION_EVENT_CHANGE:
          if (idx == This->m_index)
            This->m_events.emplace(evtype::CHANGE);
          break;
        case PA_SUBSCRIPTION_EVENT_REMOVE:
          if (idx == This->m_index)
            This->m_events.emplace(evtype::REMOVE);
          break;
      }
      break;
  }
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Simple callback to check for success
 */
void pulseaudio::simple_callback(pa_context*, int success, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  This->success = success;
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when getting sink info & existence
 */
void pulseaudio::sink_info_callback(pa_context*, const pa_sink_info* info, int eol, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  if (!eol && info) {
    This->m_index = info->index;
    This->s_name = info->name;
  }
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when context state changes
 */
void pulseaudio::context_state_callback(pa_context* context, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
      This->m_state_callback_signal = true;
      pa_threaded_mainloop_signal(This->m_mainloop, 0);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;
  }
}

inline void pulseaudio::wait_loop(pa_operation* op, pa_threaded_mainloop* loop) {
  while (pa_operation_get_state(op) != PA_OPERATION_DONE) {
    pa_threaded_mainloop_wait(loop);
  }
  pa_operation_unref(op);
}

POLYBAR_NS_END

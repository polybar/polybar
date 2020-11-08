#include "adapters/pulseaudio_source.hpp"

#include "components/logger.hpp"

POLYBAR_NS

/**
 * Construct pulseaudio object
 */
pulseaudio_source::pulseaudio_source(const logger& logger, string&& source_name, bool max_volume)
    : m_log(logger), spec_s_name(source_name) {
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

  pa_threaded_mainloop_wait(m_mainloop);
  if (pa_context_get_state(m_context) != PA_CONTEXT_READY) {
    pa_threaded_mainloop_unlock(m_mainloop);
    pa_threaded_mainloop_stop(m_mainloop);
    pa_context_disconnect(m_context);
    pa_context_unref(m_context);
    pa_threaded_mainloop_free(m_mainloop);
    throw pulseaudio_error("Could not connect to pulseaudio server.");
  }

  pa_operation* op{nullptr};
  if (!source_name.empty()) {
    op = pa_context_get_source_info_by_name(m_context, source_name.c_str(), source_info_callback, this);
    wait_loop(op, m_mainloop);
  }
  if (s_name.empty()) {
    // get the sink index
    op = pa_context_get_source_info_by_name(m_context, DEFAULT_SOURCE, source_info_callback, this);
    wait_loop(op, m_mainloop);
    m_log.notice("pulseaudio: using default source %s", s_name);
  } else {
    m_log.trace("pulseaudio: using source %s", s_name);
  }

  m_max_volume = max_volume ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;

  auto event_types = static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER);
  op = pa_context_subscribe(m_context, event_types, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to subscribe to source.");
  pa_context_set_subscribe_callback(m_context, subscribe_callback, this);

  update_volume(op);

  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Deconstruct pulseaudio
 */
pulseaudio_source::~pulseaudio_source() {
  pa_threaded_mainloop_stop(m_mainloop);
  pa_context_disconnect(m_context);
  pa_context_unref(m_context);
  pa_threaded_mainloop_free(m_mainloop);
}

/**
 * Get source name
 */
const string& pulseaudio_source::get_name() {
  return s_name;
}

/**
 * Wait for events
 */
bool pulseaudio_source::wait() {
  return m_events.size() > 0;
}

/**
 * Process queued pulseaudio events
 */
int pulseaudio_source::process_events() {
  int ret = m_events.size();
  pa_threaded_mainloop_lock(m_mainloop);
  pa_operation* o{nullptr};
  // clear the queue
  while (!m_events.empty()) {
    switch (m_events.front()) {
      // try to get specified source
      case evtype::NEW:
        // redundant if already using specified source
        if (!spec_s_name.empty()) {
          o = pa_context_get_source_info_by_name(m_context, spec_s_name.c_str(), source_info_callback, this);
          wait_loop(o, m_mainloop);
          break;
        }
        // FALLTHRU
      case evtype::SERVER:
        // don't fallthrough only if always using default source
        if (!spec_s_name.empty()) {
          break;
        }
        // FALLTHRU
      // get default source
      case evtype::REMOVE:
        o = pa_context_get_source_info_by_name(m_context, DEFAULT_SOURCE, source_info_callback, this);
        wait_loop(o, m_mainloop);
        if (spec_s_name != s_name)
          m_log.notice("pulseaudio: using default source %s", s_name);
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
int pulseaudio_source::get_volume() {
  // alternatively, user pa_cvolume_avg_mask() to average selected channels
  return static_cast<int>(pa_cvolume_max(&cv) * 100.0f / PA_VOLUME_NORM + 0.5f);
}

/**
 * Get volume in decibels
 */
double pulseaudio_source::get_decibels() {
  return pa_sw_volume_to_dB(pa_cvolume_max(&cv));
}

/**
 * Set volume to given percentage
 */
void pulseaudio_source::set_volume(float percentage) {
  pa_threaded_mainloop_lock(m_mainloop);
  pa_volume_t vol = math_util::percentage_to_value<pa_volume_t>(percentage, PA_VOLUME_MUTED, PA_VOLUME_NORM);
  pa_cvolume_scale(&cv, vol);
  pa_operation* op = pa_context_set_source_volume_by_index(m_context, m_index, &cv, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to set source volume.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Increment or decrement volume by given percentage (prevents accumulation of rounding errors from get_volume)
 */
void pulseaudio_source::inc_volume(int delta_perc) {
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
  pa_operation* op = pa_context_set_source_volume_by_index(m_context, m_index, &cv, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to set source volume.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Set mute state
 */
void pulseaudio_source::set_mute(bool mode) {
  pa_threaded_mainloop_lock(m_mainloop);
  pa_operation* op = pa_context_set_source_mute_by_index(m_context, m_index, mode, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success)
    throw pulseaudio_error("Failed to mute source.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Toggle mute state
 */
void pulseaudio_source::toggle_mute() {
  set_mute(!is_muted());
}

/**
 * Get current mute state
 */
bool pulseaudio_source::is_muted() {
  return muted;
}

/**
 * Update local volume cache
 */
void pulseaudio_source::update_volume(pa_operation* o) {
  o = pa_context_get_source_info_by_index(m_context, m_index, get_source_volume_callback, this);
  wait_loop(o, m_mainloop);
}

/**
 * Callback when getting volume
 */
void pulseaudio_source::get_source_volume_callback(pa_context*, const pa_source_info* info, int, void* userdata) {
  pulseaudio_source* This = static_cast<pulseaudio_source*>(userdata);
  if (info) {
    This->cv = info->volume;
    This->muted = info->mute;
  }
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when subscribing to changes
 */
void pulseaudio_source::subscribe_callback(pa_context*, pa_subscription_event_type_t t, uint32_t idx, void* userdata) {
  pulseaudio_source* This = static_cast<pulseaudio_source*>(userdata);
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SERVER:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_CHANGE:
          This->m_events.emplace(evtype::SERVER);
          break;
      }
      break;
    case PA_SUBSCRIPTION_EVENT_SOURCE:
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
void pulseaudio_source::simple_callback(pa_context*, int success, void* userdata) {
  pulseaudio_source* This = static_cast<pulseaudio_source*>(userdata);
  This->success = success;
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when getting sink info & existence
 */
void pulseaudio_source::source_info_callback(pa_context*, const pa_source_info* info, int eol, void* userdata) {
  pulseaudio_source* This = static_cast<pulseaudio_source*>(userdata);
  if (!eol && info) {
    This->m_index = info->index;
    This->s_name = info->name;
  }
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when context state changes
 */
void pulseaudio_source::context_state_callback(pa_context* context, void* userdata) {
  pulseaudio_source* This = static_cast<pulseaudio_source*>(userdata);
  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_READY:
    case PA_CONTEXT_TERMINATED:
    case PA_CONTEXT_FAILED:
      pa_threaded_mainloop_signal(This->m_mainloop, 0);
      break;

    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;
  }
}

inline void pulseaudio_source::wait_loop(pa_operation* op, pa_threaded_mainloop* loop) {
  while (pa_operation_get_state(op) != PA_OPERATION_DONE) pa_threaded_mainloop_wait(loop);
  pa_operation_unref(op);
}

POLYBAR_NS_END

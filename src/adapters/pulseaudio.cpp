#include "adapters/pulseaudio.hpp"

#include "components/logger.hpp"

POLYBAR_NS

/**
 * Construct pulseaudio object
 */
pulseaudio::pulseaudio(const logger& logger, devicetype device_type, string&& device_name, bool max_volume)
    : m_log(logger), spec_s_name(device_name) {
  m_device_type = device_type;
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
  if (!device_name.empty()) {
    device_update_info(device_name);
  }
  if (s_name.empty()) {
    // get the index
    device_update_info(""s);
    m_log.notice("pulseaudio: using default %s %s", get_device_type(), s_name);
  } else {
    m_log.trace("pulseaudio: using %s %s", get_device_type(), s_name);
  }

  m_max_volume = max_volume ? PA_VOLUME_UI_MAX : PA_VOLUME_NORM;

  pa_subscription_mask_t event_types;
  switch (m_device_type) {
    case devicetype::SINK:
      event_types = static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER);
      break;
    case devicetype::SOURCE:
      event_types = static_cast<pa_subscription_mask_t>(PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER);
      break;
    default:
      throw pulseaudio_error("unreachable");
  }
  op = pa_context_subscribe(m_context, event_types, simple_callback, this);
  wait_loop(op, m_mainloop);
  if (!success) {
    throw pulseaudio_error("Failed to subscribe to device.");
  }
  pa_context_set_subscribe_callback(m_context, subscribe_callback, this);

  device_update_info();

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
 * Get device type as string
 */
const string pulseaudio::get_device_type() {
  switch (m_device_type) {
    case devicetype::SINK:
      return "sink"s;
    case devicetype::SOURCE:
      return "source"s;
  }
  throw pulseaudio_error("unreachable");
}

/**
 * Wrapper around pa_context_get_XXX_info_by_name().
 */
void pulseaudio::device_update_info(string device_name) {
  pa_operation *op{nullptr};
  switch (m_device_type) {
    case devicetype::SINK:
      op = pa_context_get_sink_info_by_name(m_context,
                                            device_name.empty()?DEFAULT_SINK:device_name.c_str(),
                                            info_callback<pa_sink_info>, this);
      break;
    case devicetype::SOURCE:
      op = pa_context_get_source_info_by_name(m_context,
                                              device_name.empty()?DEFAULT_SOURCE:device_name.c_str(),
                                              info_callback<pa_source_info>, this);
      break;
  }
  wait_loop(op, m_mainloop);
}

/**
 * Wrapper around pa_context_get_XXX_info_by_index().
 */
void pulseaudio::device_update_info() {
  pa_operation *op{nullptr};
  switch (m_device_type) {
    case devicetype::SINK:
      op = pa_context_get_sink_info_by_index(m_context, m_index, get_volume_callback<pa_sink_info>, this);
      break;
    case devicetype::SOURCE:
      op = pa_context_get_source_info_by_index(m_context, m_index, get_volume_callback<pa_source_info>, this);
      break;
  }
  wait_loop(op, m_mainloop);
}

/**
 * Wrapper around pa_context_set_XXX_volume_by_index().
 */
void pulseaudio::device_set_volume() {
  pa_operation *op{nullptr};
  switch (m_device_type) {
    case devicetype::SINK:
      op = pa_context_set_sink_volume_by_index(m_context, m_index, &cv, simple_callback, this);
      break;
    case devicetype::SOURCE:
      op = pa_context_set_source_volume_by_index(m_context, m_index, &cv, simple_callback, this);
      break;
  }
  wait_loop(op, m_mainloop);
}

/**
 * Wrapper around pa_context_set_XXX_mute_by_index().
 */
void pulseaudio::device_set_mute(bool mode) {
  pa_operation *op{nullptr};
  switch (m_device_type) {
    case devicetype::SINK:
      op = pa_context_set_sink_mute_by_index(m_context, m_index, mode, simple_callback, this);
      break;
    case devicetype::SOURCE:
      op = pa_context_set_source_mute_by_index(m_context, m_index, mode, simple_callback, this);
      break;
  }
  wait_loop(op, m_mainloop);
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
  // clear the queue
  while (!m_events.empty()) {
    switch (m_events.front()) {
      // try to get specified sink/source
      case evtype::NEW:
        // redundant if already using specified sink/source
        if (!spec_s_name.empty()) {
          device_update_info(spec_s_name);
          break;
        }
        // FALLTHRU
      case evtype::SERVER:
        // don't fallthrough only if always using default sink/source
        if (!spec_s_name.empty()) {
          break;
        }
        // FALLTHRU
      // get default sink/source
      case evtype::REMOVE:
        device_update_info(""s);
        if (spec_s_name != s_name) {
          m_log.notice("pulseaudio: using default %s %s", get_device_type(), s_name);
        }
        break;
      default:
        break;
    }
    device_update_info();
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
  device_set_volume();
  if (!success)
    throw pulseaudio_error("Failed to set device volume.");
  pa_threaded_mainloop_unlock(m_mainloop);
}

/**
 * Set mute state
 */
void pulseaudio::set_mute(bool mode) {
  pa_threaded_mainloop_lock(m_mainloop);
  device_set_mute(mode);
  if (!success)
    throw pulseaudio_error("Failed to mute device.");
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
 * Callback when getting volume
 */
template <typename T>
void pulseaudio::get_volume_callback(pa_context*, const T* info, int, void* userdata) {
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
void pulseaudio::simple_callback(pa_context*, int success, void* userdata) {
  pulseaudio* This = static_cast<pulseaudio*>(userdata);
  This->success = success;
  pa_threaded_mainloop_signal(This->m_mainloop, 0);
}

/**
 * Callback when getting sink/source info & existence
 */
template <typename T>
void pulseaudio::info_callback(pa_context*, const T* info, int eol, void* userdata) {
  pulseaudio *This = static_cast<pulseaudio*>(userdata);
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

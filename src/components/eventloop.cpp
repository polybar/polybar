#include <csignal>

#include "components/eventloop.hpp"
#include "components/types.hpp"
#include "components/signals.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "x11/color.hpp"

POLYBAR_NS

/**
 * Construct eventloop instance
 */
eventloop::eventloop(const logger& logger, const config& config) : m_log(logger), m_conf(config) {
  m_delayed_time = duration_t{m_conf.get<double>("settings", "eventloop-delayed-time", 25)};
  m_swallow_time = duration_t{m_conf.get<double>("settings", "eventloop-swallow-time", 10)};
  m_swallow_limit = m_conf.get<size_t>("settings", "eventloop-swallow", 5U);

  g_signals::event::enqueue = bind(&eventloop::enqueue, this, placeholders::_1);
  g_signals::event::enqueue_delayed = bind(&eventloop::enqueue_delayed, this, placeholders::_1);
}

/**
 * Deconstruct eventloop
 */
eventloop::~eventloop() {
  g_signals::event::enqueue = g_signals::noop<const eventloop::entry_t&>;
  g_signals::event::enqueue_delayed = g_signals::noop<const eventloop::entry_t&>;

  m_update_cb = nullptr;
  m_unrecognized_input_cb = nullptr;

  if (m_delayed_thread.joinable()) {
    m_delayed_thread.join();
  }
  if (m_queue_thread.joinable()) {
    m_queue_thread.join();
  }

  for (auto&& block : m_modules) {
    for (auto&& module : block.second) {
      auto module_name = module->name();
      auto cleanup_ms = time_util::measure([&module] {
        module->stop();
        module.reset();
      });
      m_log.trace("eventloop: Deconstruction of %s took %lu microsec.", module_name, cleanup_ms);
    }
  }
}

/**
 * Start module and worker threads
 */
void eventloop::start() {
  m_log.info("Starting event loop");
  m_running = true;

  dispatch_modules();

  m_queue_thread = thread(&eventloop::dispatch_queue_worker, this);
  m_delayed_thread = thread(&eventloop::dispatch_delayed_worker, this);
}

/**
 * Wait for worker threads to end
 */
void eventloop::wait() {
  if (m_queue_thread.joinable()) {
    m_queue_thread.join();
  }
}

/**
 * Stop main loop by enqueuing a QUIT event
 */
void eventloop::stop() {
  m_log.info("Stopping event loop");
  m_running = false;

  if (m_delayed_thread.joinable()) {
    m_delayed_cond.notify_one();
  }

  enqueue({static_cast<uint8_t>(event_type::QUIT)});
}

/**
 * Enqueue event
 */
bool eventloop::enqueue(const entry_t& entry) {
  if (m_queue.enqueue(entry)) {
    return true;
  }
  m_log.warn("Failed to enqueue event (%d)", entry.type);
  return false;
}

/**
 * Delay enqueue by given time
 */
bool eventloop::enqueue_delayed(const entry_t& entry) {
  if (!m_delayed_lock.try_lock()) {
    return false;
  }

  std::unique_lock<std::mutex> guard(m_delayed_lock, std::adopt_lock);

  if (m_delayed_entry.type != 0) {
    return false;
  }

  m_delayed_entry = entry;

  if (m_queue.enqueue(entry)) {
    return true;
  }

  m_delayed_entry.type = 0;
  return false;
}

/**
 * Set callback handler for UPDATE events
 */
void eventloop::set_update_cb(callback<bool>&& cb) {
  m_update_cb = forward<decltype(cb)>(cb);
}

/**
 * Set callback handler for raw INPUT events
 */
void eventloop::set_input_db(callback<string>&& cb) {
  m_unrecognized_input_cb = forward<decltype(cb)>(cb);
}

/**
 * Add module to alignment block
 */
void eventloop::add_module(const alignment pos, module_t&& module) {
  auto it = m_modules.lower_bound(pos);

  if (it != m_modules.end() && !(m_modules.key_comp()(pos, it->first))) {
    it->second.emplace_back(forward<module_t>(module));
  } else {
    vector<module_t> vec;
    vec.emplace_back(forward<module_t>(module));
    m_modules.insert(it, modulemap_t::value_type(pos, move(vec)));
  }
}

/**
 * Get reference to module map
 */
const modulemap_t& eventloop::modules() const {
  return m_modules;
}

/**
 * Get loaded module count
 */
size_t eventloop::module_count() const {
  size_t count{0};
  for (auto&& block : m_modules) {
    count += block.second.size();
  }
  return count;
}

/**
 * Start module threads
 */
void eventloop::dispatch_modules() {
  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      try {
        m_log.info("Starting %s", module->name());
        module->start();
      } catch (const application_error& err) {
        m_log.err("Failed to start '%s' (reason: %s)", module->name(), err.what());
      }
    }
  }
}

/**
 * Dispatch queue worker thread
 */
void eventloop::dispatch_queue_worker() {
  while (m_running) {
    entry_t evt, next{static_cast<uint8_t>(event_type::NONE)};
    m_queue.wait_dequeue(evt);

    if (!m_running) {
      break;
    }

    if (m_delayed_entry.type != 0 && compare_events(evt, m_delayed_entry)) {
      m_delayed_cond.notify_one();
    }

    size_t swallowed{0};
    while (swallowed++ < m_swallow_limit && m_queue.wait_dequeue_timed(next, m_swallow_time)) {
      if (match_event(next, event_type::QUIT)) {
        evt = next;
        break;
      } else if (!compare_events(evt, next)) {
        enqueue(move(next));
        break;
      }

      m_log.trace_x("eventloop: Swallowing event within timeframe");
      evt = next;
    }

    forward_event(evt);
  }

  m_log.info("Queue worker done");
}

/**
 * Dispatch delayed worker thread
 */
void eventloop::dispatch_delayed_worker() {
  while (true) {
    // wait for notification
    while (m_running && m_delayed_entry.type != 0) {
      std::unique_lock<std::mutex> guard(m_delayed_lock);
      m_delayed_cond.wait(guard, [&] { return m_delayed_entry.type != 0 || !m_running; });
    }

    if (!m_running) {
      break;
    }

    this_thread::sleep_for(m_delayed_time);
    m_delayed_entry.type = 0;
  }

  m_log.info("Delayed worker done");
}

/**
 * Test if event matches given type
 */
inline bool eventloop::match_event(entry_t evt, event_type type) {
  return static_cast<uint8_t>(type) == evt.type;
}

/**
 * Compare given events
 */
inline bool eventloop::compare_events(entry_t evt, entry_t evt2) {
  if (evt.type != evt2.type) {
    return false;
  } else if (match_event(evt, event_type::INPUT)) {
    return evt.data[0] == evt2.data[0] && strncmp(evt.data, evt2.data, strlen(evt.data)) == 0;
  }

  return true;
}

/**
 * Forward event to handler based on type
 */
void eventloop::forward_event(entry_t evt) {
  if (evt.type == static_cast<uint8_t>(event_type::UPDATE)) {
    on_update(reinterpret_cast<const update_event&>(evt));
  } else if (evt.type == static_cast<uint8_t>(event_type::INPUT)) {
    on_input(reinterpret_cast<const input_event&>(evt));
  } else if (evt.type == static_cast<uint8_t>(event_type::CHECK)) {
    on_check();
  } else if (evt.type == static_cast<uint8_t>(event_type::QUIT)) {
    on_quit(reinterpret_cast<const quit_event&>(evt));
  } else {
    m_log.warn("Unknown event type for enqueued event (%d)", evt.type);
  }
}

/**
 * Handler for enqueued UPDATE events
 */
void eventloop::on_update(const update_event& evt) {
  m_log.trace("eventloop: Received UPDATE event");

  if (m_update_cb) {
    m_update_cb(evt.force);
  } else {
    m_log.warn("No callback to handle update");
  }
}

/**
 * Handler for enqueued INPUT events
 */
void eventloop::on_input(const input_event& evt) {
  m_log.trace("eventloop: Received INPUT event");

  for (auto&& block : m_modules) {
    for (auto&& module : block.second) {
      if (!module->receive_events()) {
        continue;
      }
      if (module->handle_event(evt.data)) {
        return;
      }
    }
  }

  if (m_unrecognized_input_cb) {
    m_unrecognized_input_cb(evt.data);
  } else {
    m_log.warn("No callback to handle unrecognized input");
  }
}

/**
 * Handler for enqueued CHECK events
 */
void eventloop::on_check() {
  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      if (m_running && module->running()) {
        return;
      }
    }
  }

  m_log.warn("No running modules...");

  stop();
}

/**
 * Handler for enqueued QUIT events
 */
void eventloop::on_quit(const quit_event& evt) {
  m_log.trace("eventloop: Received QUIT event");
  m_running = false;

  if (evt.reload) {
    kill(getpid(), SIGUSR1);
  }
}

POLYBAR_NS_END

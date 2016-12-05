#include <csignal>

#include "components/eventloop.hpp"
#include "components/types.hpp"
#include "events/signal.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "x11/color.hpp"

POLYBAR_NS

/**
 * Construct eventloop instance
 */
eventloop::eventloop(signal_emitter& emitter, const logger& logger, const config& config)
    : m_sig(emitter), m_log(logger), m_conf(config) {
  m_swallow_time = duration_t{m_conf.get<double>("settings", "eventloop-swallow-time", 10)};
  m_swallow_limit = m_conf.get<size_t>("settings", "eventloop-swallow", 5U);
  m_sig.attach(this);
}

/**
 * Deconstruct eventloop
 */
eventloop::~eventloop() {
  m_sig.detach(this);

  for (auto&& block : m_modules) {
    for (auto&& module : block.second) {
      auto module_name = module->name();
      auto cleanup_ms = time_util::measure([&module] {
        module->stop();
        module.reset();
      });
      m_log.info("eventloop: Deconstruction of %s took %lu microsec.", module_name, cleanup_ms);
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

  while (m_running) {
    event evt, next;
    input_data data;

    m_queue.wait_dequeue(evt);

    if (!m_running) {
      break;
    }

    if (evt.type == static_cast<uint8_t>(event_type::INPUT)) {
      while (m_running && m_inputqueue.try_dequeue(data)) {
        m_sig.emit(process_input{move(data)});
      }
    } else {
      size_t swallowed{0};
      while (swallowed++ < m_swallow_limit && m_queue.wait_dequeue_timed(next, m_swallow_time)) {
        if (next.type == static_cast<uint8_t>(event_type::QUIT)) {
          evt = next;
          break;

        } else if (next.type == static_cast<uint8_t>(event_type::INPUT)) {
          evt = next;
          break;

        } else if (!compare_events(evt, next)) {
          enqueue(move(next));
          break;

        } else {
          m_log.trace_x("eventloop: Swallowing event within timeframe");
          evt = next;
        }
      }

      if (evt.type == static_cast<uint8_t>(event_type::INPUT)) {
        while (m_inputqueue.try_dequeue(data)) {
          m_sig.emit(process_input{move(data)});
        }
      } else {
        forward_event(evt);
      }
    }
  }

  m_log.info("Queue worker done");
}

void eventloop::process_inputqueue() {
  input_data data{};

  while (m_inputqueue.try_dequeue(data)) {
    m_sig.emit(process_input{move(data)});
  }
}

/**
 * Stop main loop by enqueuing a QUIT event
 */
void eventloop::stop() {
  m_log.info("Stopping event loop");
  m_running = false;
  m_sig.emit(enqueue_quit{make_quit_evt(false)});
}

/**
 * Enqueue event
 */
bool eventloop::enqueue(event&& evt) {
  uint8_t type{static_cast<uint8_t>(evt.type)};

  if (!m_queue.enqueue(move(evt))) {
    m_log.warn("Failed to enqueue event (%d)", static_cast<uint8_t>(type));
    return false;
  }

  return true;
}

/**
 * Enqueue input event
 */
bool eventloop::enqueue(input_data&& evt) {
  if (!m_inputqueue.enqueue(move(evt))) {
    m_log.warn("Failed to enqueue input_data");
    return false;
  }

  return true;
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
 * Compare given events
 */
inline bool eventloop::compare_events(event evt, event evt2) {
  return evt.type != evt2.type;
}

inline bool eventloop::compare_events(input_data data, input_data data2) {
  return data.data[0] == data2.data[0] && strncmp(data.data, data2.data, strlen(data.data)) == 0;
}

/**
 * Forward event to handler based on type
 */
void eventloop::forward_event(event evt) {
  if (evt.type == static_cast<uint8_t>(event_type::QUIT)) {
    m_sig.emit(process_quit{reinterpret_cast<event&&>(evt)});
  } else if (evt.type == static_cast<uint8_t>(event_type::UPDATE)) {
    m_sig.emit(process_update{reinterpret_cast<event&&>(evt)});
  } else if (evt.type == static_cast<uint8_t>(event_type::CHECK)) {
    m_sig.emit(process_check{reinterpret_cast<event&&>(evt)});
  } else {
    m_log.warn("Unknown event type for enqueued event (%d)", evt.type);
  }
}

bool eventloop::on(const process_input& evt) {
  string input{(*evt()).data};

  for (auto&& block : m_modules) {
    for (auto&& module : block.second) {
      if (module->receive_events() && module->handle_event(input)) {
        return true;
      }
    }
  }

  m_log.warn("Input event \"%s\" was rejected by all modules, passing to shell...", input);

  return false;
}

bool eventloop::on(const process_check&) {
  for (const auto& block : m_modules) {
    for (const auto& module : block.second) {
      if (m_running && module->running()) {
        return true;
      }
    }
  }
  m_log.warn("No running modules...");
  enqueue(make_quit_evt(false));
  return true;
}

bool eventloop::on(const process_quit& evt) {
  assert((*evt()).type == static_cast<uint8_t>(event_type::QUIT));
  const event quit{static_cast<const event>(*evt())};
  m_log.info("Processing QUIT event (reload=%i)", quit.flag);
  m_running = false;
  return !quit.flag;  // break emit chain if reload flag isn't set
}

bool eventloop::on(const enqueue_event& evt) {
  m_log.trace("eventloop: enqueuing event (type=%i)", (*evt()).type);
  return enqueue(static_cast<event>(*evt()));
}

bool eventloop::on(const enqueue_quit& evt) {
  assert((*evt()).type == static_cast<uint8_t>(event_type::QUIT));
  if (m_running) {
    const event quit{reinterpret_cast<const event&>(*evt())};
    m_log.info("Enqueuing QUIT event (reload=%i)", quit.flag);
    return enqueue(static_cast<event>(*evt()));
  }
  return true;
}

bool eventloop::on(const enqueue_update& evt) {
  event update{reinterpret_cast<const event&>(*evt())};
  assert(update.type == static_cast<uint8_t>(event_type::UPDATE));
  m_log.trace("eventloop: enqueuing UPDATE event (force=%i)", update.flag);
  return enqueue(move(update));
}

bool eventloop::on(const enqueue_input& evt) {
  m_log.trace("eventloop: enqueuing INPUT event");
  return enqueue(input_data{(*evt())}) && enqueue(make_input_evt());
}

bool eventloop::on(const enqueue_check& evt) {
  event check{reinterpret_cast<const event&>(*evt())};
  assert(check.type == static_cast<uint8_t>(event_type::CHECK));
  m_log.trace("eventloop: enqueuing CHECK event");
  return enqueue(move(check));
}

POLYBAR_NS_END

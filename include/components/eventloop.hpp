#pragma once

#include <moodycamel/blockingconcurrentqueue.h>

#include "common.hpp"
#include "components/bar.hpp"
#include "components/logger.hpp"
#include "modules/meta.hpp"
#include "utils/command.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

using module_t = unique_ptr<modules::module_interface>;
using modulemap_t = map<alignment, vector<module_t>>;

enum class event_type { NONE = 0, UPDATE, CHECK, INPUT, QUIT };
struct event {
  int type;
  char data[256]{'\0'};
};

class eventloop {
 public:
  /**
   * Queue type
   */
  using entry_t = event;
  using queue_t = moodycamel::BlockingConcurrentQueue<entry_t>;

  /**
   * Construct eventloop
   */
  explicit eventloop(const logger& logger) : m_log(logger) {}

  /**
   * Deconstruct eventloop
   */
  ~eventloop() noexcept {
    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        auto module_name = module->name();
        auto cleanup_ms = time_execution([&module] {
          module->stop();
          module.reset();
        });
        m_log.trace("eventloop: Deconstruction of %s took %lu ms.", module_name, cleanup_ms);
      }
    }
  }

  /**
   * Set callback handler for UPDATE events
   */
  void set_update_cb(callback<>&& cb) {
    m_update_cb = forward<decltype(cb)>(cb);
  }

  /**
   * Set callback handler for raw INPUT events
   */
  void set_input_db(callback<string>&& cb) {
    m_unrecognized_input_cb = forward<decltype(cb)>(cb);
  }

  /**
   * Get reference to module map
   */
  modulemap_t& modules() {
    return m_modules;
  }

  /**
   * Add module to alignment block
   */
  void add_module(const alignment pos, module_t&& module) {
    modulemap_t::iterator it = m_modules.lower_bound(pos);

    if (it != m_modules.end() && !(m_modules.key_comp()(pos, it->first))) {
      it->second.emplace_back(forward<module_t>(module));
    } else {
      vector<module_t> vec;
      vec.emplace_back(forward<module_t>(module));
      m_modules.insert(it, modulemap_t::value_type(pos, move(vec)));
    }
  }

  /**
   * Enqueue event
   */
  bool enqueue(const entry_t& i) {
    bool enqueued;

    if ((enqueued = m_queue.enqueue(i)) == false) {
      m_log.warn("Failed to queue event (%d)", i.type);
    }

    return enqueued;
  }

  /**
   * Start module threads and wait for events on the queue
   *
   * @param timeframe Time to wait for subsequent events
   * @param limit Maximum amount of subsequent events to swallow within timeframe
   */
  template <typename Rep, typename Period>
  void run(chrono::duration<Rep, Period> timeframe, int limit) {
    m_log.info("Starting event loop");
    m_running = true;

    // Start module threads
    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        try {
          m_log.info("Starting %s", module->name());
          module->start();
        } catch (const application_error& err) {
          m_log.err("Failed to start '%s' (reason: %s)", module->name(), err.what());
        }
      }
    }

    m_log.trace("eventloop: Enter loop");

    while (m_running) {
      entry_t evt, next{static_cast<int>(event_type::NONE)};
      m_queue.wait_dequeue(evt);

      if (!m_running) {
        break;
      }

      if (match_event(evt, event_type::UPDATE)) {
        int swallowed = 0;
        while (swallowed++ < limit && m_queue.wait_dequeue_timed(next, timeframe)) {
          if (match_event(next, event_type::QUIT)) {
            evt = next;
            break;
          } else if (compare_events(evt, next)) {
            m_log.trace("eventloop: Swallowing event within timeframe");
            evt = next;
          } else {
            break;
          }
        }
      }

      forward_event(evt);

      if (match_event(next, event_type::NONE))
        continue;
      if (compare_events(evt, next))
        continue;

      forward_event(next);
    }

    m_log.trace("eventloop: Loop ended");
  }

  /**
   * Stop main loop by enqueuing a QUIT event
   */
  void stop() {
    m_log.info("Stopping event loop");
    m_running = false;
    enqueue({static_cast<int>(event_type::QUIT)});
  }

 protected:
  /**
   * Test if event matches given type
   */
  bool match_event(entry_t evt, event_type type) {
    return static_cast<int>(type) == evt.type;
  }

  /**
   * Compare given events
   */
  bool compare_events(entry_t evt, entry_t evt2) {
    return evt.type == evt2.type;
  }

  /**
   * Forward event to handler based on type
   */
  void forward_event(entry_t evt) {
    if (evt.type == static_cast<int>(event_type::UPDATE)) {
      on_update();
    } else if (evt.type == static_cast<int>(event_type::INPUT)) {
      on_input(string{evt.data});
    } else if (evt.type == static_cast<int>(event_type::CHECK)) {
      on_check();
    } else if (evt.type == static_cast<int>(event_type::QUIT)) {
      on_quit();
    } else {
      m_log.warn("Unknown event type for enqueued event (%d)", evt.type);
    }
  }

  /**
   * Handler for enqueued UPDATE events
   */
  void on_update() {
    m_log.trace("eventloop: Received UPDATE event");

    if (m_update_cb) {
      m_update_cb();
    } else {
      m_log.warn("No callback to handle update");
    }
  }

  /**
   * Handler for enqueued INPUT events
   */
  void on_input(string input) {
    m_log.trace("eventloop: Received INPUT event");

    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        if (!module->receive_events())
          continue;
        if (module->handle_event(input)) {
          return;
        }
      }
    }

    if (m_unrecognized_input_cb) {
      m_unrecognized_input_cb(input);
    } else {
      m_log.warn("No callback to handle unrecognized input");
    }
  }

  /**
   * Handler for enqueued CHECK events
   */
  void on_check() {
    if (!m_running) {
      return;
    }

    for (auto&& block : m_modules) {
      for (auto&& module : block.second) {
        if (module->running())
          return;
      }
    }

    m_log.warn("No running modules...");
    stop();
  }

  /**
   * Handler for enqueued QUIT events
   */
  void on_quit() {
    m_log.trace("eventloop: Received QUIT event");
    m_running = false;
  }

 private:
  const logger& m_log;

  queue_t m_queue;
  modulemap_t m_modules;
  stateflag m_running;

  callback<> m_update_cb;
  callback<string> m_unrecognized_input_cb;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<eventloop>>
  di::injector<T> configure_eventloop() {
    return di::make_injector(configure_logger());
  }
}

LEMONBUDDY_NS_END

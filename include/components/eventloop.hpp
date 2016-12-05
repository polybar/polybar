#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <chrono>

#include "common.hpp"
#include "components/logger.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

using namespace signals::eventloop;

using module_t = unique_ptr<modules::module_interface>;
using modulemap_t = map<alignment, vector<module_t>>;

class eventloop : public signal_receiver<SIGN_PRIORITY_EVENTLOOP, process_quit, process_input, process_check,
                      enqueue_event, enqueue_quit, enqueue_update, enqueue_input, enqueue_check> {
 public:
  enum class event_type : uint8_t {
    NONE = 0,
    UPDATE,
    CHECK,
    INPUT,
    QUIT,
  };

  struct event {
    uint8_t type{static_cast<uint8_t>(event_type::NONE)};
    bool flag{false};
  };

  struct input_data {
    char data[EVENT_SIZE];
  };

  template <typename EventType>
  using queue_t = moodycamel::BlockingConcurrentQueue<EventType>;
  using duration_t = chrono::duration<double, std::milli>;

 public:
  explicit eventloop(signal_emitter& emitter, const logger& logger, const config& config);
  ~eventloop();

  void start();
  void stop();

  bool enqueue(event&& evt);
  bool enqueue(input_data&& evt);

  void add_module(const alignment pos, module_t&& module);
  const modulemap_t& modules() const;
  size_t module_count() const;

  static auto make_quit_evt(bool reload = false) {
    return event{static_cast<uint8_t>(event_type::QUIT), reload};
  }
  static auto make_update_evt(bool force = false) {
    return event{static_cast<uint8_t>(event_type::UPDATE), force};
  }
  static auto make_input_evt() {
    return event{static_cast<uint8_t>(event_type::INPUT)};
  }
  static auto make_input_data(string&& data) {
    input_data d{};
    memcpy(d.data, &data[0], sizeof(d.data));
    return d;
  }
  static auto make_check_evt() {
    return event{static_cast<uint8_t>(event_type::CHECK)};
  }

 protected:
  void process_inputqueue();
  void dispatch_modules();

  inline bool compare_events(event evt, event evt2);
  inline bool compare_events(input_data data, input_data data1);

  void forward_event(event evt);

  bool on(const process_input& evt);
  bool on(const process_check& evt);
  bool on(const process_quit& evt);

  bool on(const enqueue_event& evt);
  bool on(const enqueue_quit& evt);
  bool on(const enqueue_update& evt);
  bool on(const enqueue_input& evt);
  bool on(const enqueue_check& evt);

 private:
  signal_emitter& m_sig;
  const logger& m_log;
  const config& m_conf;

  /**
   * @brief Event queue
   */
  queue_t<event> m_queue;

  /**
   * @brief Event queue
   */
  queue_t<input_data> m_inputqueue;

  /**
   * @brief Loaded modules
   */
  modulemap_t m_modules;

  /**
   * @brief Flag to indicate current run state
   */
  stateflag m_running;

  /**
   * @brief Time to wait for subsequent events
   */
  duration_t m_swallow_time{0ms};

  /**
   * @brief Maximum amount of subsequent events to swallow within timeframe
   */
  size_t m_swallow_limit{0};
};

namespace {
  inline unique_ptr<eventloop> make_eventloop() {
    return make_unique<eventloop>(make_signal_emitter(), make_logger(), make_confreader());
  }
}

POLYBAR_NS_END

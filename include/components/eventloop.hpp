#pragma once

#include <moodycamel/blockingconcurrentqueue.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "common.hpp"
#include "events/signal_emitter.hpp"
#include "events/signal_fwd.hpp"
#include "events/signal_receiver.hpp"
#include "utils/concurrency.hpp"

POLYBAR_NS

// fwd
namespace modules {
  struct module_interface;
}
enum class alignment : uint8_t;
class config;
class logger;

using namespace signals::eventloop;

using module_t = unique_ptr<modules::module_interface>;
using modulemap_t = std::map<alignment, vector<module_t>>;

namespace chrono = std::chrono;
using namespace std::chrono_literals;

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

  template <typename EventType>
  using queue_t = moodycamel::BlockingConcurrentQueue<EventType>;

 public:
  using make_type = unique_ptr<eventloop>;
  static make_type make();

  explicit eventloop(signal_emitter& emitter, const logger& logger, const config& config);
  ~eventloop();

  void start();
  void stop();

  bool enqueue(event&& evt);
  bool enqueue(string&& input_data);

  void add_module(const alignment pos, module_t&& module);
  const modulemap_t& modules() const;
  size_t module_count() const;

  static event make_quit_evt(bool reload = false);
  static event make_update_evt(bool force = false);
  static event make_input_evt();
  static event make_check_evt();

 protected:
  void dispatch_modules();

  void handle_inputdata();

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
   * @brief Loaded modules
   */
  modulemap_t m_modules;

  /**
   * @brief Flag to indicate current run state
   */
  stateflag m_running{};

  /**
   * @brief Time to wait for subsequent events
   */
  chrono::milliseconds m_swallow_update{10ms};

  /**
   * @brief Maximum amount of subsequent events to swallow within timeframe
   */
  size_t m_swallow_limit{5U};

  /**
   * @brief Time to throttle input events
   */
  chrono::milliseconds m_swallow_input{30ms};

  /**
   * @brief Time of last handled input event
   */
  chrono::time_point<chrono::system_clock, chrono::milliseconds> m_lastinput;

  /**
   * @brief Mutex used to guard input data
   */
  std::mutex m_inputlock{};

  /**
   * @brief Input data
   */
  string m_inputdata;
};

POLYBAR_NS_END

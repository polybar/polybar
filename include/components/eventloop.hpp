#pragma once

#include <moodycamel/blockingconcurrentqueue.h>
#include <chrono>

#include "common.hpp"
#include "components/logger.hpp"
#include "modules/meta/base.hpp"

POLYBAR_NS

using module_t = unique_ptr<modules::module_interface>;
using modulemap_t = map<alignment, vector<module_t>>;

enum class event_type : uint8_t {
  NONE = 0,
  UPDATE,
  CHECK,
  INPUT,
  QUIT,
};

struct event {
  uint8_t type{0};
  char data[256]{'\0'};
};

class eventloop {
 public:
  /**
   * Queue type
   */
  using entry_t = event;
  using queue_t = moodycamel::BlockingConcurrentQueue<entry_t>;
  using duration_t = chrono::duration<double, std::milli>;

  explicit eventloop(const logger& logger, const config& config);

  ~eventloop() noexcept;

  void start();
  void wait();
  void stop();

  bool enqueue(const entry_t& entry);
  bool enqueue_delayed(const entry_t& entry);

  void set_update_cb(callback<>&& cb);
  void set_input_db(callback<string>&& cb);

  void add_module(const alignment pos, module_t&& module);
  const modulemap_t& modules() const;
  size_t module_count() const;

 protected:
  void dispatch_modules();
  void dispatch_queue_worker();
  void dispatch_delayed_worker();

  inline bool match_event(entry_t evt, event_type type);
  inline bool compare_events(entry_t evt, entry_t evt2);
  void forward_event(entry_t evt);

  void on_update();
  void on_input(char* input);
  void on_check();
  void on_quit();

 private:
  const logger& m_log;
  const config& m_conf;

  /**
   * @brief Event queue
   */
  queue_t m_queue;

  /**
   * @brief Loaded modules
   */
  modulemap_t m_modules;

  /**
   * @brief Lock used when accessing the module map
   */
  std::mutex m_modulelock;

  /**
   * @brief Flag to indicate current run state
   */
  stateflag m_running;

  /**
   * @brief Callback fired when receiving UPDATE events
   */
  callback<> m_update_cb;

  /**
   * @brief Callback fired for unprocessed INPUT events
   */
  callback<string> m_unrecognized_input_cb;

  /**
   * @brief Time to wait for subsequent events
   */
  duration_t m_swallow_time{0ms};

  /**
   * @brief Maximum amount of subsequent events to swallow within timeframe
   */
  size_t m_swallow_limit{0};

  /**
   * @brief Time until releasing the lock on the delayed enqueue channel
   */
  duration_t m_delayed_time;

  /**
   * @brief Lock used to control the condition variable
   */
  std::mutex m_delayed_lock;

  /**
   * @brief Condition variable used to manage notifications for delayed enqueues
   */
  std::condition_variable m_delayed_cond;

  /**
   * @brief Pending event on the delayed channel
   */
  entry_t m_delayed_entry{0};

  /**
   * @brief Queue worker thread
   */
  thread m_queue_thread;

  /**
   * @brief Delayed notifier thread
   */
  thread m_delayed_thread;
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = unique_ptr<eventloop>>
  di::injector<T> configure_eventloop() {
    return di::make_injector(configure_logger(), configure_config());
  }
}

POLYBAR_NS_END

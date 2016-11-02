#pragma once

#include <moodycamel/blockingconcurrentqueue.h>

#include "common.hpp"
#include "components/logger.hpp"
#include "modules/meta.hpp"

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

  explicit eventloop(const logger& logger) : m_log(logger) {}

  ~eventloop() noexcept;

  bool enqueue(const entry_t& i);
  void run(chrono::duration<double, std::milli> timeframe, int limit);
  void stop();

  void set_update_cb(callback<>&& cb);
  void set_input_db(callback<string>&& cb);

  void add_module(const alignment pos, module_t&& module);

  modulemap_t& modules();

 protected:
  void start_modules();

  bool match_event(entry_t evt, event_type type);
  bool compare_events(entry_t evt, entry_t evt2);
  void forward_event(entry_t evt);

  void on_update();
  void on_input(string input);
  void on_check();
  void on_quit();

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

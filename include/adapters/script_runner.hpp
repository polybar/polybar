#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

class script_runner {
 public:
  using interval = std::chrono::duration<double>;
  script_runner(std::function<void(void)> on_update, const string& exec, const string& exec_if, bool tail,
      interval interval, const vector<pair<string, string>>& env);

  bool check_condition() const;
  interval process();

  void clear_output();

  void stop();

  int get_pid() const;
  int get_counter() const;
  int get_exit_status() const;

  string get_output();

  bool is_stopping() const;

 protected:
  bool set_output(string&&);

  interval run_tail();
  interval run();

 private:
  const logger& m_log;

  const std::function<void(void)> m_on_update;

  const string m_exec;
  const string m_exec_if;
  const bool m_tail;
  const interval m_interval;
  const vector<pair<string, string>> m_env;

  std::mutex m_output_lock;
  string m_output;

  std::atomic_int m_counter{0};
  std::atomic_bool m_stopping{false};
  std::atomic_int m_pid{-1};
  std::atomic_int m_exit_status{0};
};

POLYBAR_NS_END

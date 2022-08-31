#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "common.hpp"
#include "components/logger.hpp"

POLYBAR_NS

class script_runner {
 public:
  struct data {
    int counter{0};
    int pid{-1};
    int exit_status{0};
    string output;
  };

  using on_update = std::function<void(const data&)>;
  using interval = std::chrono::duration<double>;

  script_runner(on_update on_update, const string& exec, const string& exec_if, bool tail, interval interval_success,
      interval interval_fail, const vector<pair<string, string>>& env);

  bool check_condition() const;
  interval process();

  void clear_output();

  void stop();

  bool is_stopping() const;

 protected:
  bool set_output(string&&);
  bool set_exit_status(int);

  interval run_tail();
  interval run();

 private:
  const logger& m_log;

  const on_update m_on_update;

  const string m_exec;
  const string m_exec_if;
  const bool m_tail;
  const interval m_interval_success;
  const interval m_interval_fail;
  const vector<pair<string, string>> m_env;

  data m_data;
  std::atomic_bool m_stopping{false};
};

POLYBAR_NS_END

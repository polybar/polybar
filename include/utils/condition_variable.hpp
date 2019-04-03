#pragma once

#include "common.hpp"

#if HAVE_CLOCK_GETTIME == 0
#include <condition_variable>
#else
#include <mutex>
#include <pthread.h>
#endif

POLYBAR_NS

#if HAVE_CLOCK_GETTIME == 1

namespace stl_replacement {
  enum class cv_status { no_timeout, timeout };

  class condition_variable {
   public:
    using native_handle_type = pthread_cond_t*;

    condition_variable();
    condition_variable(const condition_variable&) = delete;
    ~condition_variable();

    void notify_one() noexcept;

    void notify_all() noexcept;

    void wait(std::unique_lock<std::mutex>& lock);

    template <typename Predicate>
    void wait(std::unique_lock<std::mutex>& lock, Predicate pred);

    template <typename Rep, typename Period>
    cv_status wait_for(std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time);

    template <typename Rep, typename Period, typename Predicate>
    bool wait_for(
        std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred);

    template <typename Clock, typename Duration>
    cv_status wait_until(
        std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& timeout_time);

    template <typename Clock, typename Duration, typename Pred>
    bool wait_until(
        std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& timeout_time, Pred pred);

    native_handle_type native_handle();

   private:
    pthread_condattr_t m_attr;
    pthread_cond_t m_handle;
  };

  inline condition_variable::condition_variable() : m_attr{}, m_handle{} {
    int result = pthread_condattr_init(&m_attr);
    if (result != 0) {
      throw std::system_error(std::error_code(result, std::generic_category()));
    }

    pthread_condattr_setclock(&m_attr, CLOCK_MONOTONIC);

    result = pthread_cond_init(&m_handle, &m_attr);
    if (result != 0) {
      throw std::system_error(std::error_code(result, std::generic_category()));
    }
  }

  inline condition_variable::~condition_variable() {
    pthread_cond_destroy(&m_handle);
    pthread_condattr_destroy(&m_attr);
  }

  inline void condition_variable::notify_one() noexcept {
    pthread_cond_signal(&m_handle);
  }

  inline void condition_variable::notify_all() noexcept {
    pthread_cond_broadcast(&m_handle);
  }

  inline void condition_variable::wait(std::unique_lock<std::mutex>& lock) {
    pthread_cond_wait(&m_handle, lock.mutex()->native_handle());
  }

  template <typename Predicate>
  void condition_variable::wait(std::unique_lock<std::mutex>& lock, Predicate pred) {
    while (!pred()) {
      wait(lock);
    }
  }

  template <typename Rep, typename Period>
  cv_status condition_variable::wait_for(
      std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time) {
    auto steady_now = std::chrono::steady_clock::now();

    auto ceiled = std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time);
    if (ceiled < rel_time) {
      ++ceiled;
    }

    auto as_seconds = std::chrono::duration_cast<std::chrono::seconds>(ceiled);
    auto ns = (ceiled - as_seconds);

    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += static_cast<decltype(ts.tv_sec)>(
        as_seconds.count() + (ts.tv_nsec + ns.count()) /
                             std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count());
    ts.tv_nsec = static_cast<decltype(ts.tv_nsec)>(
        (ts.tv_nsec + ns.count()) %
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)).count());

    auto result = pthread_cond_timedwait(&m_handle, lock.mutex()->native_handle(), &ts);
    if (result != ETIMEDOUT && result != 0) {
      throw std::system_error(std::error_code(result, std::generic_category()));
    }

    return std::chrono::steady_clock::now() - steady_now < rel_time ? cv_status::no_timeout : cv_status::timeout;
  }

  template <typename Rep, typename Period, typename Predicate>
  bool condition_variable::wait_for(
      std::unique_lock<std::mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred) {
    return wait_until(lock, rel_time, std::move(pred));
  }

  template <typename Clock, typename Duration>
  cv_status condition_variable::wait_until(
      std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& timeout_time) {
    wait_for(lock, timeout_time - Clock::now());
    return Clock::now() < timeout_time ? cv_status::no_timeout : cv_status::timeout;
  }

  template <typename Clock, typename Duration, typename Pred>
  bool condition_variable::wait_until(
      std::unique_lock<std::mutex>& lock, const std::chrono::time_point<Clock, Duration>& timeout_time, Pred pred) {
    while (!pred()) {
      if (wait_until(lock, timeout_time) == cv_status::timeout) {
        return pred();
      }
    }

    return true;
  }

  inline condition_variable::native_handle_type condition_variable::native_handle() {
    return &m_handle;
  }
}  // namespace stl_replacement

using condition_variable = stl_replacement::condition_variable;
#else
using condition_variable = std::condition_variable;
#endif

POLYBAR_NS_END

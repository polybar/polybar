#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <thread>

#include "common.hpp"
#include "settings.hpp"

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

POLYBAR_NS

enum class loglevel {
  NONE = 0,
  ERROR,
  WARNING,
  INFO,
  TRACE,
};

class logger {
 public:
  using make_type = const logger&;
  static make_type make(loglevel level = loglevel::NONE);

  explicit logger(loglevel level);

  static loglevel parse_verbosity(const string& name, loglevel fallback = loglevel::NONE);

  void verbosity(loglevel&& level);

#ifdef DEBUG_LOGGER  // {{{
  template <typename... Args>
  void trace(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#ifdef DEBUG_LOGGER_VERBOSE
  template <typename... Args>
  void trace_x(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#else
  template <typename... Args>
  void trace_x(Args...) const {}
#endif
#else
  template <typename... Args>
  void trace(Args...) const {}
  template <typename... Args>
  void trace_x(Args...) const {}
#endif  // }}}

  /**
   * Output an info message
   */
  template <typename... Args>
  void info(string message, Args... args) const {
    output(loglevel::INFO, message, args...);
  }

  /**
   * Output a warning message
   */
  template <typename... Args>
  void warn(string message, Args... args) const {
    output(loglevel::WARNING, message, args...);
  }

  /**
   * Output an error message
   */
  template <typename... Args>
  void err(string message, Args... args) const {
    output(loglevel::ERROR, message, args...);
  }

 protected:
  template <typename T>
  decltype(auto) convert(T&& arg) const {
    return forward<T>(arg);
  }

  /**
   * Convert string
   */
  const char* convert(string arg) const;  // NOLINT

  /**
   * Convert thread id
   */
  size_t convert(const std::thread::id arg) const;

  /**
   * Write the log message to the output channel
   * if the defined verbosity level allows it
   */
  template <typename... Args>
  void output(loglevel level, string format, Args... values) const {
    if (level > m_level) {
      return;
    }

#if defined(__clang__)  // {{{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif  // }}}

    dprintf(m_fd, (m_prefixes.at(level) + format + m_suffixes.at(level) + "\n").c_str(), convert(values)...);

#if defined(__clang__)  // {{{
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif  // }}}
  }

 private:
  /**
   * Logger verbosity level
   */
  loglevel m_level{loglevel::TRACE};

  /**
   * File descriptor used when writing the log messages
   */
  int m_fd{STDERR_FILENO};

  /**
   * Loglevel specific prefixes
   */
  std::map<loglevel, string> m_prefixes;

  /**
   * Loglevel specific suffixes
   */
  std::map<loglevel, string> m_suffixes;
};

POLYBAR_NS_END

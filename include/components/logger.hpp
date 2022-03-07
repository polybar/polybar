#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <thread>
#include <utility>

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
  NOTICE,
  INFO,
  TRACE,
};

class logger {
 public:
  using make_type = const logger&;
  static make_type make(loglevel level = loglevel::NONE);

  explicit logger(loglevel level);

  const logger& operator=(const logger&) const {
    return *this;
  }

  static loglevel parse_verbosity(const string& name, loglevel fallback = loglevel::NONE);

  void verbosity(loglevel level);

#ifdef DEBUG_LOGGER // {{{
  template <typename... Args>
  void trace(const string& message, Args&&... args) const {
    output(loglevel::TRACE, message, std::forward<Args>(args)...);
  }
#ifdef DEBUG_LOGGER_VERBOSE
  template <typename... Args>
  void trace_x(const string& message, Args&&... args) const {
    output(loglevel::TRACE, message, std::forward<Args>(args)...);
  }
#else
  template <typename... Args>
  void trace_x(Args&&...) const {}
#endif
#else
  template <typename... Args>
  void trace(Args&&...) const {}
  template <typename... Args>
  void trace_x(Args&&...) const {}
#endif // }}}

  /**
   * Output an info message
   */
  template <typename... Args>
  void info(const string& message, Args&&... args) const {
    output(loglevel::INFO, message, std::forward<Args>(args)...);
  }

  /**
   * Output a notice
   */
  template <typename... Args>
  void notice(const string& message, Args&&... args) const {
    output(loglevel::NOTICE, message, std::forward<Args>(args)...);
  }

  /**
   * Output a warning message
   */
  template <typename... Args>
  void warn(const string& message, Args&&... args) const {
    output(loglevel::WARNING, message, std::forward<Args>(args)...);
  }

  /**
   * Output an error message
   */
  template <typename... Args>
  void err(const string& message, Args&&... args) const {
    output(loglevel::ERROR, message, std::forward<Args>(args)...);
  }

 protected:
  template <typename T>
  decltype(auto) convert(T&& arg) const {
    return forward<T>(arg);
  }

  /**
   * Convert string
   */
  const char* convert(string& arg) const;
  const char* convert(const string& arg) const;

  /**
   * Convert thread id
   */
  size_t convert(std::thread::id arg) const;

  /**
   * Write the log message to the output channel
   * if the defined verbosity level allows it
   */
  template <typename... Args>
  void output(loglevel level, const string& format, Args&&... values) const {
    if (level > m_level) {
      return;
    }

#if defined(__clang__) // {{{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif // }}}

    dprintf(m_fd, (m_prefixes.at(level) + format + m_suffixes.at(level) + "\n").c_str(), convert(values)...);

#if defined(__clang__) // {{{
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // }}}
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

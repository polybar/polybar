#pragma once

#include <cstdio>

#include "common.hpp"

LEMONBUDDY_NS

enum class loglevel {
  NONE = 0,
  ERROR,
  WARNING,
  INFO,
  TRACE,
};

loglevel parse_loglevel_name(string name);

class logger {
 public:
  explicit logger(loglevel level);
  explicit logger(string level_name) : logger(parse_loglevel_name(level_name)) {}

  void verbosity(loglevel level);

  void verbosity(string level);

  /**
   * Output a trace message
   */
  template <typename... Args>
#ifdef DEBUG_LOGGER
  void trace(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#else
#ifdef VERBOSE_TRACELOG
#undef VERBOSE_TRACELOG
#endif
  void trace(string, Args...) const {
  }
#endif

  /**
   * Output extra verbose trace message
   */
  template <typename... Args>
#ifdef VERBOSE_TRACELOG
  void trace_x(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#else
  void trace_x(string, Args...) const {
  }
#endif

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
   * Convert string to const char*
   */
  const char* convert(string arg) const {
    return arg.c_str();
  }

  /**
   * Write the log message to the output channel
   * if the defined verbosity level allows it
   */
  template <typename... Args>
  void output(loglevel level, string format, Args... values) const {
    if (level > m_level)
      return;

    auto prefix = m_prefixes.find(level)->second;
    auto suffix = m_suffixes.find(level)->second;

// silence the compiler
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
    dprintf(m_fd, (prefix + format + suffix + "\n").c_str(), convert(values)...);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  }

 private:
  /**
   * Logger verbosity level
   */
  loglevel m_level = loglevel::TRACE;

  /**
   * File descriptor used when writing the log messages
   */
  int m_fd = STDERR_FILENO;

  /**
   * Loglevel specific prefixes
   */
  // clang-format off
  map<loglevel, string> m_prefixes {
    {loglevel::TRACE,   "lemonbuddy|trace  "},
    {loglevel::INFO,    "lemonbuddy|info   "},
    {loglevel::WARNING, "lemonbuddy|warn   "},
    {loglevel::ERROR,   "lemonbuddy|error  "},
  };

  /**
   * Loglevel specific suffixes
   */
  map<loglevel, string> m_suffixes {
    {loglevel::TRACE,   ""},
    {loglevel::INFO,    ""},
    {loglevel::WARNING, ""},
    {loglevel::ERROR,   ""},
  };
  // clang-format on
};

namespace {
  /**
   * Configure injection module
   */
  template <typename T = const logger&>
  di::injector<T> configure_logger(loglevel level = loglevel::NONE) {
    auto instance = factory::generic_singleton<logger>(level);
    return di::make_injector(di::bind<>().to(instance));
  }
}

LEMONBUDDY_NS_END

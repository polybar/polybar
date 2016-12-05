#pragma once

#include <cstdio>
#include <map>
#include <string>

#include "common.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

enum class loglevel : uint8_t {
  NONE = 0,
  ERROR,
  WARNING,
  INFO,
  TRACE,
};

loglevel parse_loglevel_name(const string& name);

class logger {
 public:
  explicit logger(loglevel level);
  explicit logger(string level_name) : logger(parse_loglevel_name(level_name)) {}

  void verbosity(loglevel level);
  void verbosity(string level);

#ifdef DEBUG_LOGGER  // {{{
  template <typename... Args>
  void trace(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#ifdef VERBOSE_TRACELOG
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

/**
 * Configure injection module
 */
namespace {
  inline const logger& make_logger(loglevel level = loglevel::NONE) {
    auto instance = factory_util::singleton<const logger>(level);
    return static_cast<const logger&>(*instance);
  }
}

POLYBAR_NS_END

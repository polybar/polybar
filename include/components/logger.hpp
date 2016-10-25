#pragma once

#include <cstdio>
#include <cstring>

#include "common.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

enum class loglevel {
  NONE = 0,
  ERROR,
  WARNING,
  INFO,
  TRACE,
};

/**
 * Convert given loglevel name to its enum type counterpart
 */
auto parse_loglevel_name = [](string name) {
  if (string_util::compare(name, "error"))
    return loglevel::ERROR;
  else if (string_util::compare(name, "warning"))
    return loglevel::WARNING;
  else if (string_util::compare(name, "info"))
    return loglevel::INFO;
  else if (string_util::compare(name, "trace"))
    return loglevel::TRACE;
  else
    return loglevel::NONE;
};

class logger {
 public:
  /**
   * Construct logger
   */
  explicit logger(loglevel level) : m_level(level) {
    if (isatty(m_fd)) {
      // clang-format off
      m_prefixes[loglevel::TRACE]   = "\r\033[0;90m- ";
      m_prefixes[loglevel::INFO]    = "\r\033[1;32m* \033[0m";
      m_prefixes[loglevel::WARNING] = "\r\033[1;33mwarning: \033[0m";
      m_prefixes[loglevel::ERROR]   = "\r\033[1;31merror: \033[0m";

      m_suffixes[loglevel::TRACE]   = "\033[0m";
      m_suffixes[loglevel::INFO]    = "\033[0m";
      m_suffixes[loglevel::WARNING] = "\033[0m";
      m_suffixes[loglevel::ERROR]   = "\033[0m";
      // clang-format on
    }
  }

  /**
   * Construct logger
   */
  explicit logger(string level_name) : logger(parse_loglevel_name(level_name)) {}

  /**
   * Set output verbosity
   */
  void verbosity(loglevel level) {
#ifndef DEBUG
    if (level == loglevel::TRACE)
      throw application_error("not a debug build: trace disabled...");
#endif
    m_level = level;
  }

  /**
   * Set output verbosity by loglevel name
   */
  void verbosity(string level) {
    verbosity(parse_loglevel_name(level));
  }

  /**
   * Output a trace message
   */
  template <typename... Args>
#ifdef DEBUG
  void trace(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#else
  void trace(string, Args...) const {}
#endif

  /**
   * Output extra verbose trace message
   */
  template <typename... Args>
#ifdef ENABLE_VERBOSE_TRACELOG
  void trace_x(string message, Args... args) const {
    output(loglevel::TRACE, message, args...);
  }
#else
  void trace_x(string, Args...) const {}
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

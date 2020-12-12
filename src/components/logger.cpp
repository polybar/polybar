#include "components/logger.hpp"

#include <unistd.h>

#include "errors.hpp"
#include "settings.hpp"
#include "utils/concurrency.hpp"
#include "utils/factory.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Convert string
 */
const char* logger::convert(string& arg) const {
  return arg.c_str();
}

const char* logger::convert(const string& arg) const {
  return arg.c_str();
}

/**
 * Convert thread id
 */
size_t logger::convert(const std::thread::id arg) const {
  return concurrency_util::thread_id(arg);
}

/**
 * Create instance
 */
logger::make_type logger::make(loglevel level) {
  return *factory_util::singleton<std::remove_reference_t<logger::make_type>>(level);
}

/**
 * Construct logger
 */
logger::logger(loglevel level) : m_level(level) {
  // clang-format off
  if (isatty(m_fd)) {
    m_prefixes[loglevel::TRACE]   = "\r\033[0;32m- \033[0m";
    m_prefixes[loglevel::INFO]    = "\r\033[1;32m* \033[0m";
    m_prefixes[loglevel::NOTICE]  = "\r\033[1;34mnotice: \033[0m";
    m_prefixes[loglevel::WARNING] = "\r\033[1;33mwarn: \033[0m";
    m_prefixes[loglevel::ERROR]   = "\r\033[1;31merror: \033[0m";
    m_suffixes[loglevel::TRACE]   = "\033[0m";
    m_suffixes[loglevel::INFO]    = "\033[0m";
    m_suffixes[loglevel::NOTICE]  = "\033[0m";
    m_suffixes[loglevel::WARNING] = "\033[0m";
    m_suffixes[loglevel::ERROR]   = "\033[0m";
  } else {
    m_prefixes.emplace(make_pair(loglevel::TRACE,   "polybar|trace: "));
    m_prefixes.emplace(make_pair(loglevel::INFO,    "polybar|info:  "));
    m_prefixes.emplace(make_pair(loglevel::NOTICE,  "polybar|notice:  "));
    m_prefixes.emplace(make_pair(loglevel::WARNING, "polybar|warn:  "));
    m_prefixes.emplace(make_pair(loglevel::ERROR,   "polybar|error: "));
    m_suffixes.emplace(make_pair(loglevel::TRACE,   ""));
    m_suffixes.emplace(make_pair(loglevel::INFO,    ""));
    m_suffixes.emplace(make_pair(loglevel::NOTICE,  ""));
    m_suffixes.emplace(make_pair(loglevel::WARNING, ""));
    m_suffixes.emplace(make_pair(loglevel::ERROR,   ""));
  }
  // clang-format on
}

/**
 * Set output verbosity
 */
void logger::verbosity(loglevel level) {
#ifndef DEBUG_LOGGER
  if (level == loglevel::TRACE) {
    throw application_error("Trace logging is not enabled...");
  }
#endif
  m_level = level;
}

/**
 * Convert given loglevel name to its enum type counterpart
 */
loglevel logger::parse_verbosity(const string& name, loglevel fallback) {
  if (string_util::compare(name, "error")) {
    return loglevel::ERROR;
  } else if (string_util::compare(name, "warning")) {
    return loglevel::WARNING;
  } else if (string_util::compare(name, "notice")) {
    return loglevel::NOTICE;
  } else if (string_util::compare(name, "info")) {
    return loglevel::INFO;
  } else if (string_util::compare(name, "trace")) {
    return loglevel::TRACE;
  } else {
    return fallback;
  }
}

POLYBAR_NS_END

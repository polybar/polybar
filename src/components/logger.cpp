#include "components/logger.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

/**
 * Construct logger
 */
logger::logger(loglevel level) : m_level(level) {
  if (isatty(m_fd)) {
    // clang-format off
    m_prefixes[loglevel::TRACE]   = "\r\033[0;90m- ";
    m_prefixes[loglevel::INFO]    = "\r\033[1;32m* \033[0m";
    m_prefixes[loglevel::WARNING] = "\r\033[1;33mwarn: \033[0m";
    m_prefixes[loglevel::ERROR]   = "\r\033[1;31merror: \033[0m";
    m_suffixes[loglevel::TRACE]   = "\033[0m";
    m_suffixes[loglevel::INFO]    = "\033[0m";
    m_suffixes[loglevel::WARNING] = "\033[0m";
    m_suffixes[loglevel::ERROR]   = "\033[0m";
    // clang-format on
  }
}

/**
 * Set output verbosity
 */
void logger::verbosity(loglevel level) {
#ifndef DEBUG
  if (level == loglevel::TRACE)
    throw application_error("not a debug build: trace disabled...");
#endif
  m_level = level;
}

/**
 * Set output verbosity by loglevel name
 */
void logger::verbosity(string level) {
  verbosity(parse_loglevel_name(level));
}

/**
 * Convert given loglevel name to its enum type counterpart
 */
loglevel parse_loglevel_name(string name) {
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

LEMONBUDDY_NS_END

#include "components/logger.hpp"
#include <unistd.h>
#include <sstream>
#include "errors.hpp"
#include "settings.hpp"
#include "utils/concurrency.hpp"
#include "utils/factory.hpp"
#include "utils/string.hpp"

POLYBAR_NS

/**
 * Convert string to string_view for safety and modern C++ style.
 */
std::string_view logger::convert(const std::string& arg) const {
  return arg;
}

/**
 * Convert thread ID to string for portability.
 */
std::string logger::convert(const std::thread::id arg) const {
  std::ostringstream oss;
  oss << arg;
  return oss.str();
}

/**
 * Create instance of logger using singleton pattern.
 */
logger::make_type logger::make(loglevel level) {
  return *factory_util::singleton<std::remove_reference_t<logger::make_type>>(level);
}

/**
 * Logger constructor.
 */
logger::logger(loglevel level) : m_level(level) {
  // Ensure m_fd is initialized before using isatty
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
    m_prefixes[loglevel::TRACE]   = "polybar|trace: ";
    m_prefixes[loglevel::INFO]    = "polybar|info:  ";
    m_prefixes[loglevel::NOTICE]  = "polybar|notice:  ";
    m_prefixes[loglevel::WARNING] = "polybar|warn:  ";
    m_prefixes[loglevel::ERROR]   = "polybar|error: ";
    m_suffixes[loglevel::TRACE]   = "";
    m_suffixes[loglevel::INFO]    = "";
    m_suffixes[loglevel::NOTICE]  = "";
    m_suffixes[loglevel::WARNING] = "";
    m_suffixes[loglevel::ERROR]   = "";
  }
}

/**
 * Set output verbosity level with better runtime checks.
 */
void logger::verbosity(loglevel level) {
  if (level == loglevel::TRACE && !is_trace_enabled()) {
    throw application_error("Trace logging is not enabled...");
  }
  m_level = level;
}

/**
 * Convert log level name to its enum counterpart using a map.
 */
loglevel logger::parse_verbosity(const std::string& name, loglevel fallback) {
  static const std::unordered_map<std::string, loglevel> level_map{
    {"error", loglevel::ERROR},
    {"warning", loglevel::WARNING},
    {"notice", loglevel::NOTICE},
    {"info", loglevel::INFO},
    {"trace", loglevel::TRACE}
  };
  
  auto it = level_map.find(name);
  return (it != level_map.end()) ? it->second : fallback;
}

POLYBAR_NS_END

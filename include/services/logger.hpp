#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <unistd.h>

#include "exception.hpp"
#include "utils/streams.hpp"

#define LOGGER_FD STDERR_FILENO

#define LOGGER_TAG_FATAL isatty(LOGGER_FD) ? "\033[1;31mfatal\033[37m ** \033[0m" : "fatal ** "
#define LOGGER_TAG_ERR   isatty(LOGGER_FD) ? "\033[1;31m  err\033[37m ** \033[0m" : "error ** "
#define LOGGER_TAG_WARN  isatty(LOGGER_FD) ? "\033[1;33m warn\033[37m ** \033[0m" : "warn  ** "
#define LOGGER_TAG_INFO  isatty(LOGGER_FD) ? "\033[1;36m info\033[37m ** \033[0m" : "info  ** "
#define LOGGER_TAG_DEBUG isatty(LOGGER_FD) ? "\033[1;35mdebug\033[37m ** \033[0m" : "debug ** "
#define LOGGER_TAG_TRACE isatty(LOGGER_FD) ? "\033[1;32mtrace\033[37m ** \033[0m" : "trace ** "

#define log_error(s) get_logger()->error(s)
#define log_warning(s) get_logger()->warning(s)
#define log_info(s) get_logger()->info(s)
#define log_debug(s) get_logger()->debug(s)
#ifdef DEBUG
#define log_trace(s) get_logger()->trace(__FILE__, __FUNCTION__, __LINE__, s)
#else
#define log_trace(s) if (0) {}
#endif

enum LogLevel
{
  LEVEL_NONE    = 1 << 0,
  LEVEL_ERROR   = 1 << 1,
  LEVEL_WARNING = 1 << 2,
  LEVEL_INFO    = 1 << 4,
  LEVEL_DEBUG   = 1 << 8,
  LEVEL_TRACE   = 1 << 16,
#ifdef DEBUG
  LEVEL_ALL     = LEVEL_ERROR | LEVEL_WARNING | LEVEL_INFO | LEVEL_DEBUG | LEVEL_TRACE
#else
  LEVEL_ALL     = LEVEL_ERROR | LEVEL_WARNING | LEVEL_INFO | LEVEL_DEBUG
#endif
};

class Logger
{
  std::mutex mtx;

  int level = LogLevel::LEVEL_ERROR | LogLevel::LEVEL_WARNING | LogLevel::LEVEL_INFO;
  int fd = LOGGER_FD;

  void output(std::string tag, std::string msg);

  public:
    Logger();

    void set_level(int level);
    void add_level(int level);

    void fatal(std::string msg);
    void error(std::string msg);
    void warning(std::string msg);
    void info(std::string msg);
    void debug(std::string msg);

    void fatal(int msg)   { fatal(std::to_string(msg)); }
    void error(int msg)   { error(std::to_string(msg)); }
    void warning(int msg) { warning(std::to_string(msg)); }
    void info(int msg)    { info(std::to_string(msg)); }
    void debug(int msg)   { debug(std::to_string(msg)); }

    void trace(const char *file, const char *fn, int lineno, std::string msg);
    void trace(const char *file, const char *fn, int lineno, int msg) {
      trace(file, fn, lineno, std::to_string(msg));
    }
};

std::shared_ptr<Logger> &get_logger();

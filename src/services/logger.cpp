#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include "services/logger.hpp"
#include "utils/string.hpp"
#include "utils/proc.hpp"

std::shared_ptr<Logger> logger;
std::shared_ptr<Logger> &get_logger()
{
  if (logger == nullptr)
    logger = std::make_unique<Logger>();
  return logger;
}

Logger::Logger()
{
  if (isatty(LOGGER_FD)) {
    dup2(LOGGER_FD, this->fd);
  }
}

// void Logger::set_level(int mask)
// {
//   this->level = mask;
// }

void Logger::add_level(int mask)
{
  this->level |= mask;
}

void Logger::fatal(const std::string& msg)
{
  dprintf(this->fd, "%s%s\n", LOGGER_TAG_FATAL, msg.c_str());
  std::exit(EXIT_FAILURE);
}

void Logger::error(const std::string& msg)
{
  if (this->level & LogLevel::LEVEL_ERROR)
    dprintf(this->fd, "%s%s\n", LOGGER_TAG_ERR, msg.c_str());
}

void Logger::warning(const std::string& msg)
{
  if (this->level & LogLevel::LEVEL_WARNING)
    dprintf(this->fd, "%s%s\n", LOGGER_TAG_WARN, msg.c_str());
}

void Logger::info(const std::string& msg)
{
  if (this->level & LogLevel::LEVEL_INFO)
    dprintf(this->fd, "%s%s\n", LOGGER_TAG_INFO, msg.c_str());
}

void Logger::debug(const std::string& msg)
{
  if (this->level & LogLevel::LEVEL_DEBUG)
    dprintf(this->fd, "%s%s\n", LOGGER_TAG_DEBUG, msg.c_str());
}

void Logger::trace(const char *file, const char *fn, int lineno, const std::string& msg)
{
  if (this->level & LogLevel::LEVEL_TRACE && !msg.empty())
    dprintf(this->fd, "%s%s (%s:%d) -> %s\n", LOGGER_TAG_TRACE, &file[0], fn, lineno, msg.c_str());
  else if (msg.empty())
    dprintf(this->fd, "%s%s (%s:%d)\n", LOGGER_TAG_TRACE, &file[0], fn, lineno);
}

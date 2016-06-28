#include <stdio.h>
#include <iostream>
#include <unistd.h>

#include "services/logger.hpp"
#include "utils/string.hpp"
#include "utils/proc.hpp"

std::shared_ptr<Logger> logger;
std::shared_ptr<Logger> get_logger()
{
  if (logger == nullptr)
    logger = std::make_shared<Logger>();
  return logger;
}

Logger::Logger()
{
  if (isatty(LOGGER_FD))
    dup2(LOGGER_FD, this->fd);
}

void Logger::set_level(int mask) {
  this->level = mask;
}

void Logger::add_level(int mask) {
  this->level |= mask;
}

void Logger::output(std::string tag, std::string msg) {
  dprintf(this->fd, "%s%s\n", tag.c_str(), msg.c_str());
}

void Logger::fatal(std::string msg)
{
  this->output(LOGGER_TAG_FATAL, msg);
  std::exit(EXIT_FAILURE);
}

void Logger::error(std::string msg)
{
  if (this->level & LogLevel::LEVEL_ERROR)
    this->output(LOGGER_TAG_ERR, msg);
}

void Logger::warning(std::string msg)
{
  if (this->level & LogLevel::LEVEL_WARNING)
    this->output(LOGGER_TAG_WARN, msg);
}

void Logger::info(std::string msg)
{
  if (this->level & LogLevel::LEVEL_INFO)
    this->output(LOGGER_TAG_INFO, msg);
}

void Logger::debug(std::string msg)
{
  if (this->level & LogLevel::LEVEL_DEBUG)
    this->output(LOGGER_TAG_DEBUG, msg);
}

void Logger::trace(const char *file, const char *fn, int lineno, std::string msg)
{
#ifdef DEBUG
  if (!(this->level & LogLevel::LEVEL_TRACE))
    return;
  char trace_msg[1024];
  if (!msg.empty())
    sprintf(trace_msg, "%s (%s:%d) -> %s", &file[0], fn, lineno, msg.c_str());
  else
    sprintf(trace_msg, "%s (%s:%d)", &file[0], fn, lineno);
  this->output(LOGGER_TAG_TRACE, trace_msg);
#endif
}

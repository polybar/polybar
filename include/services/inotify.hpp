#pragma once

#include <memory>
#include <sys/inotify.h>

#include "exception.hpp"

class InotifyException : public Exception
{
  public:
    explicit InotifyException(std::string msg)
      : Exception("[Inotify] "+ msg){}
};

struct InotifyEvent
{
  static const auto ACCESSED = IN_ACCESS;
  static const auto MODIFIED = IN_MODIFY;
  static const auto ATTRIBUTES_CHANGED = IN_ATTRIB;

  static const auto FILE_ADDED = IN_CREATE;
  static const auto FILE_REMOVED = IN_DELETE;

  static const auto DELETED_SELF = IN_DELETE_SELF;
  static const auto MOVED_SELF = IN_MOVE_SELF;

  static const auto MOVED_FROM = IN_MOVED_FROM;
  static const auto MOVED_TO = IN_MOVED_TO;

  static const auto OPENED = IN_OPEN;
  static const auto CLOSED = IN_CLOSE;
  static const auto MOVED = IN_MOVE;

  static const auto ALL = (ACCESSED | MODIFIED | ATTRIBUTES_CHANGED \
                           | FILE_ADDED | FILE_REMOVED \
                           | DELETED_SELF | MOVED_SELF \
                           | MOVED_FROM | MOVED_TO \
                           | OPENED | CLOSED | MOVED);

  bool is_dir;
  std::string filename;
  int wd, cookie, mask = 0;
};

class InotifyWatch
{
  std::string path;
  int fd = -1, wd = -1, mask;

  public:
    explicit InotifyWatch(std::string path, int mask = InotifyEvent::ALL);
    ~InotifyWatch();

    std::string operator()() {
      return this->path;
    }

    bool has_event(int timeout_ms = 1000);
    std::unique_ptr<InotifyEvent> get_event();
};

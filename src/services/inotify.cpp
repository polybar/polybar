#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "services/inotify.hpp"
#include "services/logger.hpp"
#include "utils/io.hpp"
#include "utils/macros.hpp"
#include "utils/proc.hpp"

InotifyWatch::InotifyWatch(std::string path, int mask)
{
  log_trace("Installing watch at: "+ path);

  if ((this->fd = inotify_init()) < 0)
    throw InotifyException(StrErrno());
  if ((this->wd = inotify_add_watch(this->fd, path.c_str(), mask)) < 0)
    throw InotifyException(StrErrno());

  this->path = path;
  this->mask = mask;
}

InotifyWatch::~InotifyWatch()
{
  log_trace("Uninstalling watch at: "+ this->path);

  if ((this->fd > 0 || this->wd > 0) && inotify_rm_watch(this->fd, this->wd) == -1)
    log_error("Failed to remove inotify watch: "+ StrErrno());

  if (this->fd > 0 && close(this->fd) == -1)
    log_error("Failed to close inotify watch fd: "+ StrErrno());
}

bool InotifyWatch::has_event(int timeout_ms) {
  return io::poll_read(this->fd, timeout_ms);
}

std::unique_ptr<InotifyEvent> InotifyWatch::get_event()
{
  char buffer[1024];
  std::size_t bytes = read(this->fd, buffer, 1024), len = 0;
  auto event = std::make_unique<InotifyEvent>();

  while (len < bytes) {
    struct inotify_event *e = (struct inotify_event *) &buffer[len];

    if (e->len) {
      event->filename = e->name;
    } else {
      event->filename = this->path;
    }

    event->wd = e->wd;
    event->cookie = e->cookie;
    event->is_dir = e->mask & IN_ISDIR;
    event->mask |= e->mask;

    len += sizeof(struct inotify_event) + e->len;
  }

  return event;
}

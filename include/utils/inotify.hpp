#pragma once

#include <sys/inotify.h>
#include <sys/poll.h>
#include <cstdio>

#include "common.hpp"
#include "utils/memory.hpp"

LEMONBUDDY_NS

using inotify_event_t = struct inotify_event;

struct inotify_event {
  string filename;
  bool is_dir;
  int wd = 0;
  int cookie = 0;
  int mask = 0;
};

class inotify_watch {
 public:
  /**
   * Constructor
   */
  explicit inotify_watch(string path) : m_path(path) {}

  /**
   * Destructor
   */
  ~inotify_watch() noexcept {
    if (m_wd != -1)
      inotify_rm_watch(m_fd, m_wd);
    if (m_fd != -1)
      close(m_fd);
  }

  /**
   * Attach inotify watch
   */
  void attach(int mask = IN_MODIFY) {
    if (m_fd == -1 && (m_fd = inotify_init()) == -1)
      throw system_error("Failed to allocate inotify fd");
    if ((m_wd = inotify_add_watch(m_fd, m_path.c_str(), mask)) == -1)
      throw system_error("Failed to attach inotify watch");
  }

  /**
   * Remove inotify watch
   */
  void remove() {
    if (inotify_rm_watch(m_fd, m_wd) == -1)
      throw system_error("Failed to remove inotify watch");
    m_wd = -1;
  }

  /**
   * Poll the inotify fd for events
   *
   * @brief A wait_ms of -1 blocks until an event is fired
   */
  bool poll(int wait_ms = 1000) {
    if (m_fd == -1)
      return false;

    struct pollfd fds[1];
    fds[0].fd = m_fd;
    fds[0].events = POLLIN;

    ::poll(fds, 1, wait_ms);

    return fds[0].revents & POLLIN;
  }

  /**
   * Get the latest inotify event
   */
  unique_ptr<inotify_event> get_event() {
    auto event = make_unique<inotify_event>();

    if (m_fd == -1 || m_wd == -1)
      return event;

    char buffer[1024];
    size_t bytes = read(m_fd, buffer, 1024);
    size_t len = 0;

    while (len < bytes) {
      auto* e = reinterpret_cast<inotify_event_t*>(&buffer[len]);

      event->filename = e->len ? e->name : m_path;
      event->wd = e->wd;
      event->cookie = e->cookie;
      event->is_dir = e->mask & IN_ISDIR;
      event->mask |= e->mask;

      len += sizeof(inotify_event_t) + e->len;
    }

    return event;
  }

  /**
   * Get watch file path
   */
  const string path() const {
    return m_path;
  }

 protected:
  string m_path;
  int m_fd = -1;
  int m_wd = -1;
};

using inotify_watch_t = unique_ptr<inotify_watch>;

namespace inotify_util {
  inline auto make_watch(string path) {
    di::injector<inotify_watch_t> injector = di::make_injector(di::bind<>().to(path));
    return injector.create<inotify_watch_t>();
  }
}

LEMONBUDDY_NS_END

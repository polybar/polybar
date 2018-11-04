#include <unistd.h>

#include "errors.hpp"
#include "utils/inotify.hpp"
#include "utils/memory.hpp"

POLYBAR_NS

/**
 * Construct inotify watch
 */
inotify_watch::inotify_watch(string path) : m_path(move(path)) {}

/**
 * Deconstruct inotify watch
 */
inotify_watch::~inotify_watch() {
  if (m_wd != -1) {
    inotify_rm_watch(m_fd, m_wd);
  }
  if (m_fd != -1) {
    close(m_fd);
  }
}

/**
 * Attach inotify watch
 */
void inotify_watch::attach(int mask) {
  if (m_fd == -1 && (m_fd = inotify_init()) == -1) {
    throw system_error("Failed to allocate inotify fd");
  }
  if ((m_wd = inotify_add_watch(m_fd, m_path.c_str(), mask)) == -1) {
    throw system_error("Failed to attach inotify watch");
  }
  m_mask |= mask;
}

/**
 * Remove inotify watch
 */
void inotify_watch::remove(bool force) {
  if (inotify_rm_watch(m_fd, m_wd) == -1 && !force) {
    throw system_error("Failed to remove inotify watch");
  }
  m_wd = -1;
  m_mask = 0;
}

/**
 * Poll the inotify fd for events
 *
 * \brief A wait_ms of -1 blocks until an event is fired
 */
bool inotify_watch::poll(int wait_ms) const {
  if (m_fd == -1) {
    return false;
  }

  struct pollfd fds[1];
  fds[0].fd = m_fd;
  fds[0].events = POLLIN;

  ::poll(fds, 1, wait_ms);

  return fds[0].revents & POLLIN;
}

/**
 * Get the latest inotify event
 */
unique_ptr<inotify_event> inotify_watch::get_event() const {
  auto event = factory_util::unique<inotify_event>();

  if (m_fd == -1 || m_wd == -1) {
    return event;
  }

  char buffer[1024];
  auto bytes = read(m_fd, buffer, 1024);
  auto len = 0;

  while (len < bytes) {
    auto* e = reinterpret_cast<::inotify_event*>(&buffer[len]);

    event->filename = e->len ? e->name : m_path;
    event->wd = e->wd;
    event->cookie = e->cookie;
    event->is_dir = e->mask & IN_ISDIR;
    event->mask |= e->mask;

    len += sizeof(*e) + e->len;
  }

  return event;
}

/**
 * Wait for matching event
 */
unique_ptr<inotify_event> inotify_watch::await_match() const {
  auto event = get_event();
  return event->mask & m_mask ? std::move(event) : nullptr;
}

/**
 * Get watch file path
 */
const string inotify_watch::path() const {
  return m_path;
}

/**
 * Get the file descriptor associated with the watch
 */
int inotify_watch::get_file_descriptor() const {
  return m_fd;
}

POLYBAR_NS_END

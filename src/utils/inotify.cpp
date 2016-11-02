#include "utils/inotify.hpp"
#include "utils/memory.hpp"

LEMONBUDDY_NS

namespace inotify_util {
  /**
   * Destructor
   */
  inotify_watch::~inotify_watch() noexcept {
    if (m_wd != -1)
      inotify_rm_watch(m_fd, m_wd);
    if (m_fd != -1)
      close(m_fd);
  }

  /**
   * Attach inotify watch
   */
  void inotify_watch::attach(int mask) {
    if (m_fd == -1 && (m_fd = inotify_init()) == -1)
      throw system_error("Failed to allocate inotify fd");
    if ((m_wd = inotify_add_watch(m_fd, m_path.c_str(), mask)) == -1)
      throw system_error("Failed to attach inotify watch");
  }

  /**
   * Remove inotify watch
   */
  void inotify_watch::remove() {
    if (inotify_rm_watch(m_fd, m_wd) == -1)
      throw system_error("Failed to remove inotify watch");
    m_wd = -1;
  }

  /**
   * Poll the inotify fd for events
   *
   * @brief A wait_ms of -1 blocks until an event is fired
   */
  bool inotify_watch::poll(int wait_ms) {
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
  unique_ptr<event_t> inotify_watch::get_event() {
    auto event = make_unique<event_t>();

    if (m_fd == -1 || m_wd == -1)
      return event;

    char buffer[1024];
    size_t bytes = read(m_fd, buffer, 1024);
    size_t len = 0;

    while (len < bytes) {
      auto* e = reinterpret_cast<::inotify_event*>(&buffer[len]);

      event->filename = e->len ? e->name : m_path;
      event->wd = e->wd;
      event->cookie = e->cookie;
      event->is_dir = e->mask & IN_ISDIR;
      event->mask |= e->mask;

      len += sizeof(event_t) + e->len;
    }

    return event;
  }

  /**
   * Get watch file path
   */
  const string inotify_watch::path() const {
    return m_path;
  }

  watch_t make_watch(string path) {
    di::injector<watch_t> injector = di::make_injector(di::bind<>().to(path));
    return injector.create<watch_t>();
  }
}

LEMONBUDDY_NS_END

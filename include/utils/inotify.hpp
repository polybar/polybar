#pragma once

#include <poll.h>
#include <sys/inotify.h>
#include <cstdio>

#include "common.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

struct inotify_event {
  string filename;
  bool is_dir;
  int wd = 0;
  int cookie = 0;
  int mask = 0;
};

class inotify_watch {
 public:
  explicit inotify_watch(string path);
  ~inotify_watch();

  void attach(int mask = IN_MODIFY);
  void remove(bool force = false);
  bool poll(int wait_ms = 1000);
  unique_ptr<inotify_event> get_event();
  bool await_match();
  const string path() const;
  int get_file_descriptor() const;

 protected:
  string m_path;
  int m_fd = -1;
  int m_wd = -1;
  int m_mask = 0;
};

namespace inotify_util {
  bool match(const inotify_event* evt, int mask);

  template <typename... Args>
  decltype(auto) make_watch(Args&&... args) {
    return factory_util::unique<inotify_watch>(forward<Args>(args)...);
  }
}

POLYBAR_NS_END

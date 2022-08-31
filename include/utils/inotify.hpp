#pragma once

#include <poll.h>
#include <sys/inotify.h>

#include <cstdio>

#include "common.hpp"

POLYBAR_NS

struct inotify_event {
  bool is_valid = false;
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
  bool poll(int wait_ms = 1000) const;
  inotify_event get_event() const;
  const string path() const;
  int get_file_descriptor() const;

 protected:
  string m_path;
  int m_fd{-1};
  int m_wd{-1};
  int m_mask{0};
};

POLYBAR_NS_END

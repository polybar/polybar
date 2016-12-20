#pragma once

#include "common.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

class file_ptr {
 public:
  explicit file_ptr(const string& path, const string& mode = "a+");
  ~file_ptr();

  file_ptr(const file_ptr& o) = delete;
  file_ptr& operator=(const file_ptr& o) = delete;

  operator bool();

  FILE* operator()();

 protected:
  FILE* m_ptr = nullptr;
  string m_path;
  string m_mode;
};

class file_descriptor {
 public:
  explicit file_descriptor(const string& path, int flags = 0);
  explicit file_descriptor(int fd);
  ~file_descriptor();
  operator int();

 protected:
  int m_fd{0};
};

namespace file_util {
  bool exists(const string& filename);
  string get_contents(const string& filename);
  void set_block(int fd);
  void set_nonblock(int fd);
  bool is_fifo(string filename);

  template <typename... Args>
  decltype(auto) make_file_descriptor(Args&&... args) {
    return factory_util::shared<file_descriptor>(forward<Args>(args)...);
  }
}

POLYBAR_NS_END

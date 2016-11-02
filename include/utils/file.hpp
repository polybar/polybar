#pragma once

#include "common.hpp"

LEMONBUDDY_NS

namespace file_util {
  /**
   * RAII file wrapper
   */
  class file_ptr {
   public:
    explicit file_ptr(const string& path, const string& mode = "a+")
        : m_path(string(path)), m_mode(string(mode)) {
      m_ptr = fopen(m_path.c_str(), m_mode.c_str());
    }

    ~file_ptr();

    operator bool();

    FILE* operator()();

   protected:
    FILE* m_ptr = nullptr;
    string m_path;
    string m_mode;
  };

  bool exists(string filename);
  string get_contents(string filename);
  void set_block(int fd);
  void set_nonblock(int fd);
  bool is_fifo(string filename);
}

LEMONBUDDY_NS_END

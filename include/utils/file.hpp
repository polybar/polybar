#pragma once

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fstream>

#include "common.hpp"
#include "utils/scope.hpp"

LEMONBUDDY_NS

namespace file_util {
  /**
   * RAII file wrapper
   */
  class file_ptr {
   public:
    /**
     * Constructor: open file handler
     */
    explicit file_ptr(const string& path, const string& mode = "a+")
        : m_path(string(path)), m_mode(string(mode)) {
      m_ptr = fopen(m_path.c_str(), m_mode.c_str());
    }

    /**
     * Destructor: close file handler
     */
    ~file_ptr() {
      if (m_ptr != nullptr)
        fclose(m_ptr);
    }

    /**
     * Logical operator testing if the file handler was created
     */
    operator bool() {
      return m_ptr != nullptr;
    }

    /**
     * Call operator returning a pointer to the file handler
     */
    FILE* operator()() {
      return m_ptr;
    }

   protected:
    FILE* m_ptr = nullptr;
    string m_path;
    string m_mode;
  };

  /**
   * Checks if the given file exist
   */
  inline auto exists(string filename) {
    struct stat buffer;
    return stat(filename.c_str(), &buffer) == 0;
  }

  /**
   * Gets the contents of the given file
   */
  inline auto get_contents(string filename) {
    try {
      std::ifstream ifs(filename);
      string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
      return contents;
    } catch (std::ios_base::failure& e) {
      return string{""};
    }
  }

  /**
   * Puts the given file descriptor into blocking mode
   */
  inline auto set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
      throw system_error("Failed to unset O_NONBLOCK");
  }

  /**
   * Puts the given file descriptor into non-blocking mode
   */
  inline auto set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
      throw system_error("Failed to set O_NONBLOCK");
  }

  /**
   * Checks if the given file is a named pipe
   */
  inline auto is_fifo(string filename) {
    auto fileptr = make_unique<file_ptr>(filename);
    int fd = fileno((*fileptr)());
    struct stat statbuf;
    fstat(fd, &statbuf);
    return S_ISFIFO(statbuf.st_mode);
  }
}

LEMONBUDDY_NS_END

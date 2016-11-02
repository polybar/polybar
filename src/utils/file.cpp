#include "utils/file.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fstream>
#include "utils/scope.hpp"

LEMONBUDDY_NS

namespace file_util {
  /**
   * Destructor: close file handler
   */
  file_ptr::~file_ptr() {
    if (m_ptr != nullptr)
      fclose(m_ptr);
  }

  /**
   * Logical operator testing if the file handler was created
   */
  file_ptr::operator bool() {
    return m_ptr != nullptr;
  }

  /**
   * Call operator returning a pointer to the file handler
   */
  FILE* file_ptr::operator()() {
    return m_ptr;
  }

  /**
   * Checks if the given file exist
   */
  bool exists(string filename) {
    struct stat buffer;
    return stat(filename.c_str(), &buffer) == 0;
  }

  /**
   * Gets the contents of the given file
   */
  string get_contents(string filename) {
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
  void set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
      throw system_error("Failed to unset O_NONBLOCK");
  }

  /**
   * Puts the given file descriptor into non-blocking mode
   */
  void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
      throw system_error("Failed to set O_NONBLOCK");
  }

  /**
   * Checks if the given file is a named pipe
   */
  bool is_fifo(string filename) {
    auto fileptr = make_unique<file_ptr>(filename);
    int fd = fileno((*fileptr)());
    struct stat statbuf;
    fstat(fd, &statbuf);
    return S_ISFIFO(statbuf.st_mode);
  }
}

LEMONBUDDY_NS_END

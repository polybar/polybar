#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <fstream>

#include "errors.hpp"
#include "utils/file.hpp"

POLYBAR_NS

/**
 * Deconstruct file wrapper
 */
file_ptr::file_ptr(const string& path, const string& mode) : m_path(string(path)), m_mode(string(mode)) {
  m_ptr = fopen(m_path.c_str(), m_mode.c_str());
}

/**
 * Deconstruct file wrapper
 */
file_ptr::~file_ptr() {
  if (m_ptr != nullptr) {
    fclose(m_ptr);
  }
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
 * Construct file descriptor wrapper
 */
file_descriptor::file_descriptor(const string& path, int flags) {
  if ((m_fd = open(path.c_str(), flags)) == -1) {
    throw system_error("Failed to open file descriptor");
  }
}

/**
 * Construct file descriptor wrapper from an existing handle
 */
file_descriptor::file_descriptor(int fd) : m_fd(fd) {}

/**
 * Deconstruct file descriptor wrapper
 */
file_descriptor::~file_descriptor() {
  if (m_fd > 0) {
    close(m_fd);
  }
}

/**
 * Conversion operator returning the fd handle
 */
file_descriptor::operator int() {
  return m_fd;
}

namespace file_util {
  /**
   * Checks if the given file exist
   */
  bool exists(const string& filename) {
    struct stat buffer {};
    return stat(filename.c_str(), &buffer) == 0;
  }

  /**
   * Gets the contents of the given file
   */
  string get_contents(const string& filename) {
    try {
      std::ifstream ifs(filename);
      string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
      return contents;
    } catch (std::ios_base::failure& e) {
      return string{""};
    }
  }

  /**
   * Checks if the given file is a named pipe
   */
  bool is_fifo(string filename) {
    auto fileptr = factory_util::unique<file_ptr>(filename);
    int fd = fileno((*fileptr)());
    struct stat statbuf {};
    fstat(fd, &statbuf);
    return S_ISFIFO(statbuf.st_mode);
  }
}

POLYBAR_NS_END

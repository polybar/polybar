#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <fstream>
#include <streambuf>

#include "errors.hpp"
#include "utils/file.hpp"

POLYBAR_NS

// implementation of file_ptr {{{

file_ptr::file_ptr(const string& path, const string& mode) : m_path(string(path)), m_mode(string(mode)) {
  m_ptr = fopen(m_path.c_str(), m_mode.c_str());
}

file_ptr::~file_ptr() {
  if (m_ptr != nullptr) {
    fclose(m_ptr);
  }
}

file_ptr::operator bool() {
  return static_cast<const file_ptr&>(*this);
}
file_ptr::operator bool() const {
  return m_ptr != nullptr;
}

file_ptr::operator FILE*() {
  return static_cast<const file_ptr&>(*this);
}
file_ptr::operator FILE*() const {
  return m_ptr;
}

file_ptr::operator int() {
  return static_cast<const file_ptr&>(*this);
}
file_ptr::operator int() const {
  return fileno(*this);
}

// }}}
// implementation of file_descriptor {{{

file_descriptor::file_descriptor(const string& path, int flags) {
  if ((m_fd = open(path.c_str(), flags)) == -1) {
    throw system_error("Failed to open file descriptor");
  }
}

file_descriptor::file_descriptor(int fd, bool autoclose) : m_fd(fd), m_autoclose(autoclose) {
  if (m_fd != -1 && !*this) {
    throw system_error("Given file descriptor (" + to_string(m_fd) + ") is not valid");
  }
}

file_descriptor::~file_descriptor() {
  if (m_autoclose) {
    close();
  }
}

file_descriptor& file_descriptor::operator=(const int fd) {
  if (m_autoclose) {
    close();
  }
  m_fd = fd;
  return *this;
}

file_descriptor::operator int() {
  return static_cast<const file_descriptor&>(*this);
}
file_descriptor::operator int() const {
  return m_fd;
}

file_descriptor::operator bool() {
  return static_cast<const file_descriptor&>(*this);
}
file_descriptor::operator bool() const {
  errno = 0;  // reset since fcntl only changes it on error
  if ((fcntl(m_fd, F_GETFD) == -1) || errno == EBADF) {
    errno = EBADF;
    return false;
  }
  return true;
}

void file_descriptor::close() {
  if (m_fd != -1 && ::close(m_fd) == -1) {
    throw system_error("Failed to close file descriptor");
  }
  m_fd = -1;
}

// }}}
// implementation of file_streambuf {{{

fd_streambuf::~fd_streambuf() {
  close();
}

fd_streambuf::operator int() {
  return static_cast<const fd_streambuf&>(*this);
}
fd_streambuf::operator int() const {
  return m_fd;
}

void fd_streambuf::open(int fd) {
  if (m_fd) {
    close();
  }
  m_fd = fd;
  setg(m_in, m_in, m_in);
  setp(m_out, m_out + sizeof(m_in));
}

void fd_streambuf::close() {
  if (m_fd) {
    sync();
    m_fd = -1;
  }
}

int fd_streambuf::sync() {
  if (pbase() != pptr()) {
    auto size = pptr() - pbase();
    auto bytes = ::write(m_fd, m_out, size);
    if (bytes > 0) {
      std::copy(pbase() + bytes, pptr(), pbase());
      setp(pbase(), epptr());
      pbump(size - bytes);
    }
  }
  return pptr() != epptr() ? 0 : -1;
}

int fd_streambuf::overflow(int c) {
  if (!traits_type::eq_int_type(c, traits_type::eof())) {
    *pptr() = traits_type::to_char_type(c);
    pbump(1);
  }
  return sync() == -1 ? traits_type::eof() : traits_type::not_eof(c);
}

int fd_streambuf::underflow() {
  if (gptr() == egptr()) {
    std::streamsize pback(std::min(gptr() - eback(), std::ptrdiff_t(m_in)));
    std::copy(egptr() - pback, egptr(), eback());
    int bytes(::read(m_fd, eback() + pback, BUFSIZ));
    setg(eback(), eback() + pback, eback() + pback + std::max(0, bytes));
  }
  return gptr() == egptr() ? traits_type::eof() : traits_type::to_int_type(*gptr());
}

// }}}

namespace file_util {
  /**
   * Checks if the given file exist
   */
  bool exists(const string& filename) {
    struct stat buffer {};
    return stat(filename.c_str(), &buffer) == 0;
  }

  /**
   * Picks the first existing file out of given entries
   */
  string pick(const vector<string>& filenames) {
    for (auto&& f : filenames) {
      if (exists(f)) {
        return f;
      }
    }
    return "";
  }

  /**
   * Gets the contents of the given file
   */
  string contents(const string& filename) {
    try {
      string contents;
      std::getline(std::ifstream(filename, std::ifstream::in), contents);
      return contents;
    } catch (const std::ifstream::failure& e) {
      return "";
    }
  }

  /**
   * Checks if the given file is a named pipe
   */
  bool is_fifo(const string& filename) {
    struct stat buffer {};
    return stat(filename.c_str(), &buffer) == 0 && S_ISFIFO(buffer.st_mode);
  }
}

POLYBAR_NS_END

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

file_descriptor::file_descriptor(int fd) : m_fd(fd) {
  if (!*this) {
    throw system_error("Given file descriptor is not valid");
  }
}

file_descriptor::~file_descriptor() {
  close();
}

file_descriptor& file_descriptor::operator=(const int fd) {
  close();
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
  if (m_fd == -1) {
    return;
  } else if (::close(m_fd) == -1) {
    throw system_error("Failed to close file descriptor");
  }
  m_fd = -1;
}

// }}}
// implementation of file_streambuf {{{

fd_streambuf::fd_streambuf(int fd) : m_fd(fd) {}

fd_streambuf::~fd_streambuf() {
  sync();
}

fd_streambuf::operator int() {
  return static_cast<const fd_streambuf&>(*this);
}
fd_streambuf::operator int() const {
  return m_fd;
}

void fd_streambuf::open(int fd) {
  if (m_fd) {
    sync();
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
    auto pback = std::min(gptr() - eback(), std::ptrdiff_t(m_in));
    std::copy(egptr() - pback, egptr(), eback());
    auto bytes = ::read(m_fd, eback() + pback, BUFSIZ);
    setg(eback(), eback() + pback, eback() + pback + std::max(0L, bytes));
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
      std::ifstream ifs(filename);
      return string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    } catch (std::ios_base::failure& e) {
      return "";
    }
  }

  /**
   * Checks if the given file is a named pipe
   */
  bool is_fifo(string filename) {
    auto fileptr = factory_util::unique<file_ptr>(filename);
    int fd = fileno(*fileptr);
    struct stat statbuf {};
    fstat(fd, &statbuf);
    return S_ISFIFO(statbuf.st_mode);
  }
}

POLYBAR_NS_END

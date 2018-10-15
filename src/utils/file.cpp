#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <streambuf>

#include "errors.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"
#include "utils/string.hpp"

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
  setp(m_out, m_out + bufsize - 1);
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
    auto bytes = write(m_fd, m_out, size);
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
    std::streamsize pback(std::min(gptr() - eback(), std::ptrdiff_t(16 - sizeof(int))));
    std::copy(egptr() - pback, egptr(), eback());
    int bytes(read(m_fd, eback() + pback, bufsize));
    setg(eback(), eback() + pback, eback() + pback + std::max(0, bytes));
  }
  return gptr() == egptr() ? traits_type::eof() : traits_type::to_int_type(*gptr());
}

// }}}

namespace file_util {
  namespace {
    void expand_home_dir(string& path) {
      if (path.empty() || path[0] != '~')
        return;

      auto pos = path.find('/', 1);
      if (pos == string::npos)
        pos = path.length();

      if (pos == 1)
      {
        path.replace(0, 1, env_util::get("HOME"));
        return;
      }

      auto username = path.substr(1, pos - 1); // username = path[1:pos]

      struct passwd pwd;
      char buffer[4096];
      struct passwd *result;

      if (getpwnam_r(username.c_str(), &pwd, buffer, sizeof(buffer), &result) != 0)
        throw system_error("getpwnam_r failed");

      if (result == nullptr)
        throw application_error("could not find user \"" + username + '"');

      path.replace(0, pos, result->pw_dir);
    }
  }
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
      string line;
      std::ifstream in(filename, std::ifstream::in);
      while (std::getline(in, line)) {
        contents += line + '\n';
      }
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

  /*
   * Checks if the (potentionally non-canonical) path is absolute
   */
  bool is_absolute(const string& filename) {
    // this is sufficient for Unix-like systems
    return !filename.empty() && (filename[0] == '/' || filename[0] == '~');
  }

  /**
   * Get glob results using given pattern
   */
  vector<string> glob(string pattern) {
    glob_t result{};
    vector<string> ret;

    // Manually expand tilde to fix builds using versions of libc
    // that doesn't provide the GLOB_TILDE flag (musl for example)
    if (pattern[0] == '~')
      file_util::expand_home_dir(pattern);

    if (::glob(pattern.c_str(), 0, nullptr, &result) == 0) {
      for (size_t i = 0_z; i < result.gl_pathc; ++i) {
        ret.emplace_back(result.gl_pathv[i]);
      }
      globfree(&result);
    }

    return ret;
  }

  /*
   * Return the path without the last component (see dirname(3))
   */
  const string dirname(const string& path) {
    if (path.empty())
      return ".";

    auto start = path.size() - 1;
    if (path[start] == '/' && start > 0)
      --start;

    auto pos = path.rfind('/', start);
    if (pos == string::npos)
      return ".";
    if (pos == 0) // path == "/" or path == "/directory"
      return "/";

    return path.substr(0, pos);
  }

  /**
   * Path expansion
   */
  const string expand(const string& path) {
    auto ret = path;
    if (ret[0] == '~')
      file_util::expand_home_dir(ret);

    vector<string> p_exploded = string_util::split(ret, '/');
    for (auto it = p_exploded.begin(); it != p_exploded.end(); ) {
      if (it->empty() || *it == ".")
        it = p_exploded.erase(it);
      else if (*it == "..")
      {
        if (it != p_exploded.begin())
        {
          --it;
          it = p_exploded.erase(it);
        }
        it = p_exploded.erase(it);
      }
      else
      {
        auto& path = *it;

        // subtract 1 since we are not interested when the last character is '$'
        for (auto pos = path.find('$'); pos < path.length() - 1; pos = path.find('$', pos + 1))
        {
          auto replace_start = pos++;

          string::size_type replace_end, count;
          if (path[pos] == '{')
          {
            ++pos;
            count = path.find('}', pos + 1);
            if (count == string::npos)
              break;
            count -= pos;
            replace_end = pos + (count + 1);
          }
          else
          {
            count = path.length() - pos;
            replace_end = pos + count;
          }

          auto env = env_util::get(path.substr(pos, count));
          path.replace(replace_start, replace_end, env);

          pos += count;
          if ( pos >= path.length() - 1)
            break;
        }

        // erase any separators at the end (due to expansion) to avoid double '//'
        while (!path.empty() && path.back() == '/')
            path.pop_back();

        // remove separators at the start to avoid double '//'
        string::size_type start = 0;
        while (start < path.length() && path[start] == '/')
            ++start;
        if (start > 0)
            path = path.substr(start);

        ++it;
      }
    }

    if (p_exploded.empty())
        return "/";

    ret = string_util::join(p_exploded, "/");

    // if the original path had a trailing separator, preserve it
    if (path.back() == '/')
      ret += '/';

    // if the path is not absolute, insert a separator at the start
    if (ret[0] != '/')
      ret.insert(0, 1, '/');

    return ret;
  }
}

POLYBAR_NS_END

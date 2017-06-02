#pragma once

#include <streambuf>

#include "common.hpp"
#include "utils/factory.hpp"

POLYBAR_NS

class file_ptr {
 public:
  explicit file_ptr(const string& path, const string& mode = "a+");
  ~file_ptr();

  explicit operator bool();
  operator bool() const;

  explicit operator FILE*();
  operator FILE*() const;

  explicit operator int();
  operator int() const;

 private:
  FILE* m_ptr = nullptr;
  string m_path;
  string m_mode;
};

class file_descriptor {
 public:
  explicit file_descriptor(const string& path, int flags = 0);
  explicit file_descriptor(int fd, bool autoclose = true);
  ~file_descriptor();

  file_descriptor& operator=(const int);

  explicit operator bool();
  operator bool() const;

  explicit operator int();
  operator int() const;

 protected:
  void close();

 private:
  int m_fd{-1};
  bool m_autoclose{true};
};

class fd_streambuf : public std::streambuf {
 public:
  using traits_type = std::streambuf::traits_type;

  template <typename... Args>
  explicit fd_streambuf(Args&&... args) : m_fd(forward<Args>(args)...) {
    setg(m_in, m_in, m_in);
    setp(m_out, m_out + bufsize - 1);
  }
  ~fd_streambuf();

  explicit operator int();
  operator int() const;

  void open(int fd);
  void close();

 protected:
  int sync();
  int overflow(int c);
  int underflow();

 private:
  file_descriptor m_fd;
  enum { bufsize = 1024 };
  char m_out[bufsize];
  char m_in[bufsize - 1];
};

template <typename StreamType>
class fd_stream : public StreamType {
 public:
  using type = fd_stream<StreamType>;

  template <typename... Args>
  explicit fd_stream(Args&&... args) : StreamType(nullptr), m_buf(forward<Args>(args)...) {
    StreamType::rdbuf(&m_buf);
  }

  explicit operator int() {
    return static_cast<const type&>(*this);
  }

  operator int() const {
    return m_buf;
  }

 protected:
  fd_streambuf m_buf;
};

namespace file_util {
  bool exists(const string& filename);
  string pick(const vector<string>& filenames);
  string contents(const string& filename);
  bool is_fifo(const string& filename);
  vector<string> glob(string pattern);
  const string expand(const string& path);

  template <typename... Args>
  decltype(auto) make_file_descriptor(Args&&... args) {
    return factory_util::unique<file_descriptor>(forward<Args>(args)...);
  }
}

POLYBAR_NS_END

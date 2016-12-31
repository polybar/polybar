#pragma once

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
  explicit file_descriptor(int fd);
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
};

class fd_streambuf : public std::streambuf {
 public:
  using traits_type = std::streambuf::traits_type;

  explicit fd_streambuf(int fd);
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
  char m_out[BUFSIZ]{'\0'};
  char m_in[BUFSIZ - 1]{'\0'};
};

template <typename StreamType = std::ostream>
class fd_stream : public StreamType {
 public:
  using type = fd_stream<StreamType>;

  explicit fd_stream(int fd) : m_buf(fd) {
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
  bool is_fifo(string filename);

  template <typename... Args>
  decltype(auto) make_file_descriptor(Args&&... args) {
    return factory_util::unique<file_descriptor>(forward<Args>(args)...);
  }
}

POLYBAR_NS_END

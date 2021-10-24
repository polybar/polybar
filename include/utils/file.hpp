#pragma once

#include <streambuf>

#include "common.hpp"

POLYBAR_NS

class file_descriptor {
 public:
  explicit file_descriptor(const string& path, int flags = 0, bool autoclose = true);
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
    setg(m_in, m_in + BUFSIZE_IN, m_in + BUFSIZE_IN);
    setp(m_out, m_out + BUFSIZE_OUT - 1);
  }
  ~fd_streambuf();

  explicit operator int();
  operator int() const;

  void open(int fd);
  void close();

 protected:
  int sync() override;
  int overflow(int c) override;
  int underflow() override;

 private:
  static constexpr int BUFSIZE_OUT = 1024;
  static constexpr int BUFSIZE_IN = 1024;
  file_descriptor m_fd;
  char m_out[BUFSIZE_OUT];
  char m_in[BUFSIZE_IN];
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
  bool is_file(const string& filename);
  bool is_dir(const string& filename);
  string pick(const vector<string>& filenames);
  string contents(const string& filename);
  void write_contents(const string& filename, const string& contents);
  bool is_fifo(const string& filename);
  vector<string> glob(string pattern);
  string expand(const string& path, const string& relative_to = {});
  string get_config_path();
  vector<string> list_files(const string& dirname);
  string dirname(const string& path);

  template <typename... Args>
  decltype(auto) make_file_descriptor(Args&&... args) {
    return std::make_unique<file_descriptor>(forward<Args>(args)...);
  }
}  // namespace file_util

POLYBAR_NS_END

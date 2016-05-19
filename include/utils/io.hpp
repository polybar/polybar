#ifndef _UTILS_IO_HPP_
#define _UTILS_IO_HPP_

#include <string>
#include <functional>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace io
{
  namespace socket
  {
    int open(const std::string& path);
    int send(int fd, const std::string& data, int flags = 0);
    int recv(int fd, char *buffer, int recv_bytes, int flags = 0);
  }

  namespace file
  {
    class FilePtr
    {
      FILE *fptr = nullptr;

      public:
        std::string path;
        std::string mode;

        FilePtr(const std::string& path, const std::string& mode = "a+")
        {
          this->path = std::string(path);
          this->mode = std::string(mode);
          this->fptr = fopen(this->path.c_str(), this->mode.c_str());
        }

        ~FilePtr()
        {
          if (this->fptr != nullptr)
            fclose(this->fptr);
        }

        operator bool() {
          return this->fptr != nullptr;
        }

        FILE *operator()() {
          return this->fptr;
        }
    };

    bool exists(const std::string& fname);
    std::string get_contents(const std::string& fname);
    bool is_fifo(const std::string& fname);
    std::size_t write(FilePtr *fptr, const std::string& data);
    std::size_t write(const std::string& fpath, const std::string& data);
  }

  std::string read(int read_fd, int bytes_to_read = -1);
  std::string read(int read_fd, int bytes_to_read, int &bytes_read_loc, int &status_loc);
  std::string readline(int read_fd);

  int write(int write_fd, const std::string& data);
  int writeline(int write_fd, const std::string& data);

  void tail(int read_fd, std::function<void(std::string)> callback);
  void tail(int read_fd, int writeback_fd);

  bool poll_read(int fd, int timeout_ms = 1);
  bool poll_write(int fd, int timeout_ms = 1);
  bool poll(int fd, short int events, int timeout_ms = 1);

  int get_flags(int fd);
  int set_blocking(int fd);
  int set_non_blocking(int fd);
}

#endif

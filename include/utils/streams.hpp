#if 0
#pragma once

#include <iostream>
#include <streambuf>
#include <cstdio>
#include <unistd.h>

namespace streams
{
  class fdoutbuf : public std::streambuf
  {
    protected:
      int fd;

      virtual int_type overflow(int_type c)
      {
        if (c != EOF) {
          char z = c;
          if (write (fd, &z, 1) != 1)
            return EOF;
        }

        return c;
      }

      virtual std::streamsize xsputn(const char* s, std::streamsize num) {
        return write(fd,s,num);
      }

    public:
      fdoutbuf(int _fd) : fd(_fd) {}
  };

  class fdout : public std::ostream
  {
    protected:
      fdoutbuf buf;

    public:
      fdout(int fd) : std::ostream(0), buf(fd) {
        rdbuf(&buf);
      }
  };
}
#endif

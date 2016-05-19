#ifndef _SERVICES_STORE_HPP_
#define _SERVICES_STORE_HPP_

#include <string>
#include <memory>

#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

#define PIPE_READ  0
#define PIPE_WRITE 1

struct Store
{
  const char FLAG_NO_DATA = '1';
  const char FLAG_DATA = '2';

  boost::interprocess::mapped_region region;
  boost::interprocess::mapped_region state_region;

  void flag();
  void unflag();
  bool check();

  char &get(char &d);
  void set(char val);

  std::string get_string();
  std::string &get_string(std::string& s);
  void set_string(const std::string& s);

  Store(int size);
  ~Store() {}
};

#endif

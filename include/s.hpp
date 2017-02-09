#pragma once

#include <cerrno>
#include <cstring>
#include <mutex>

#include "common.hpp"

POLYBAR_NS

class Singleton
{
 public:
  static Singleton& getInstance()
  {
    static Singleton  instance;
    return instance;
  }
  std::string data;
  std::mutex mtx;
 private:
  Singleton() {}
  Singleton(Singleton const&);              // Don't Implement.
  void operator=(Singleton const&); // Don't implement
};

POLYBAR_NS_END
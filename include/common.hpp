#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "settings.hpp"

#define POLYBAR_NS    \
  namespace polybar {
#define POLYBAR_NS_END \
  }

#ifndef PIPE_READ
#define PIPE_READ 0
#endif
#ifndef PIPE_WRITE
#define PIPE_WRITE 1
#endif

POLYBAR_NS

using std::string;
using std::size_t;
using std::move;
using std::forward;
using std::pair;
using std::function;
using std::shared_ptr;
using std::unique_ptr;
using std::make_unique;
using std::make_shared;
using std::make_pair;
using std::array;
using std::vector;
using std::to_string;

using namespace std::string_literals;

constexpr size_t operator"" _z(unsigned long long n) {
  return n;
}

POLYBAR_NS_END

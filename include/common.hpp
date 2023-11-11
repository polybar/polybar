#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "settings.hpp"

#define POLYBAR_NS namespace polybar {
#define POLYBAR_NS_END }

#ifndef PIPE_READ
#define PIPE_READ 0
#endif
#ifndef PIPE_WRITE
#define PIPE_WRITE 1
#endif

POLYBAR_NS

using std::array;
using std::forward;
using std::function;
using std::make_pair;
using std::make_shared;
using std::make_unique;
using std::move;
using std::pair;
using std::shared_ptr;
using std::size_t;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

using namespace std::string_literals;

constexpr size_t operator"" _z(unsigned long long n) {
  return n;
}

/**
 * Convert an enum to its underlying type.
 */
template <typename E>
constexpr auto to_integral(E e) {
  static_assert(std::is_enum<E>::value, "only enums are supported");
  return static_cast<typename std::underlying_type_t<E>>(e);
}

POLYBAR_NS_END

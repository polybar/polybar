#pragma once

#include "common.hpp"

POLYBAR_NS

/**
 * Base class for non copyable objects
 */
class non_copyable_mixin {
 public:
  non_copyable_mixin(const non_copyable_mixin&) = delete;
  non_copyable_mixin& operator=(const non_copyable_mixin&) = delete;

 protected:
  non_copyable_mixin() = default;
  ~non_copyable_mixin() = default;
};

/**
 * Base class for non movable objects
 */
class non_movable_mixin {
 public:
  non_movable_mixin(non_movable_mixin&&) = delete;
  non_movable_mixin& operator=(non_movable_mixin&&) = delete;

 protected:
  non_movable_mixin() = default;
  ~non_movable_mixin() = default;
};

POLYBAR_NS_END

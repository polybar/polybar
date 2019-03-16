#pragma once

#include "common.hpp"

POLYBAR_NS

/**
 * Base class for non copyable objects
 */
template <class T>
class non_copyable_mixin {
 protected:
  non_copyable_mixin() = default;
  ~non_copyable_mixin() = default;

 private:
  non_copyable_mixin(const non_copyable_mixin&) = delete;
  non_copyable_mixin& operator=(const non_copyable_mixin&) = delete;
};

/**
 * Base class for non movable objects
 */
template <class T>
class non_movable_mixin {
 protected:
  non_movable_mixin() = default;
  ~non_movable_mixin() = default;

 private:
  non_movable_mixin(non_movable_mixin&&) = delete;
  non_movable_mixin& operator=(non_movable_mixin&&) = delete;
};

POLYBAR_NS_END

#pragma once

#include <cstdlib>

#include "common.hpp"

POLYBAR_NS

template <typename T>
using malloc_unique_ptr = unique_ptr<T, decltype(free)*>;

POLYBAR_NS_END

#pragma once

#include <functional>

#include "common.hpp"

POLYBAR_NS

template <typename... Args>
using callback = function<void(Args...)>;

POLYBAR_NS_END

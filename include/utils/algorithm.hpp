#pragma once

#include <algorithm>

#include "common.hpp"

POLYBAR_NS

namespace algo_util {
    template <typename T>
    std::vector<T> sort_in_place(std::vector<T>&& vector) {
      std::sort(vector.begin(), vector.end());
      return move(vector);
    }

    template <typename T, typename Comp>
    std::vector<T>&& sort_in_place(std::vector<T>&& vector, Comp&& comp) {
      std::sort(vector.begin(), vector.end(), std::forward<Comp>(comp));
      return move(vector);
    }
}

POLYBAR_NS_END

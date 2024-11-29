#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "errors.hpp"

POLYBAR_NS

DEFINE_ERROR(value_error);
DEFINE_ERROR(key_error);

using valuemap_t = std::unordered_map<string, string>;
using sectionmap_t = std::map<string, valuemap_t>;
using file_list = vector<string>;

namespace config_utils {
  template <typename T>
  T convert(string&& value);
};

POLYBAR_NS_END

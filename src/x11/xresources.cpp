#include "x11/xresources.hpp"

#include "common.hpp"

POLYBAR_NS

template <>
string xresource_manager::convert(string&& value) const {
  return forward<string>(value);
}

template <>
double xresource_manager::convert(string&& value) const {
  return std::strtod(value.c_str(), nullptr);
}

POLYBAR_NS_END

#pragma once

#include <iomanip>

#include "common.hpp"
#include "utils/string.hpp"

LEMONBUDDY_NS

namespace color_util {
  template <typename ChannelType = uint8_t, typename ValueType = uint32_t>
  struct color {
    using type = color<ChannelType, ValueType>;
    union {
      struct {
        ChannelType red;
        ChannelType green;
        ChannelType blue;
        ChannelType alpha;
      } bits;

      ValueType value = 0U;
    } colorspace;

    explicit color(ValueType v) {
      colorspace.value = v;
    }
  };

  template <typename T>
  auto make_24bit(T&& value) {
    return color<uint8_t, uint32_t>(forward<T>(value));
  }

  template <typename T>
  auto make_32bit(T&& value) {
    return color<uint16_t, uint32_t>(forward<T>(value));
  }

  template <typename ValueType = uint32_t>
  uint8_t alpha(const color<ValueType> c) {
    return ((c.colorspace.value >> 24) << 8) | ((c.colorspace.value >> 24));
  }

  template <typename T = uint8_t, typename ValueType = uint32_t>
  T red(const color<ValueType> c) {
    uint8_t r = c.colorspace.value >> 16;
    if (std::is_same<T, uint8_t>::value)
      return r << 8 / 0xFF;
    if (std::is_same<T, uint16_t>::value)
      return r << 8 | r << 8 / 0xFF;
  }

  template <typename T = uint8_t, typename ValueType = uint32_t>
  T green(const color<ValueType> c) {
    uint8_t g = c.colorspace.value >> 8;
    if (std::is_same<T, uint8_t>::value)
      return g << 8 / 0xFF;
    if (std::is_same<T, uint16_t>::value)
      return g << 8 | g << 8 / 0xFF;
  }

  template <typename T = uint8_t, typename ValueType = uint32_t>
  T blue(const color<ValueType> c) {
    uint8_t b = c.colorspace.value;
    if (std::is_same<T, uint8_t>::value)
      return b << 8 / 0xFF;
    if (std::is_same<T, uint16_t>::value)
      return b << 8 | b << 8 / 0xFF;
  }

  string hex(const color<uint8_t, uint32_t> value) {
    // clang-format off
    return string_util::from_stream(stringstream()
        << "#"
        << std::setw(6)
        << std::setfill('0')
        << std::hex
        << std::uppercase
        << (value.colorspace.value & 0x00FFFFFF));
    // clang-format on
  }

  string hex(const color<uint16_t, uint32_t> value) {
    // clang-format off
    return string_util::from_stream(stringstream()
        << "#"
        << std::setw(8)
        << std::setfill('0')
        << std::hex
        << std::uppercase
        << value.colorspace.value);
    // clang-format on
  }
}

LEMONBUDDY_NS_END

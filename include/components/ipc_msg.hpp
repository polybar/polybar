#pragma once

#include "common.hpp"

POLYBAR_NS

namespace ipc {
  /**
   * Magic string prefixed to every ipc message.
   *
   * THIS MUST NEVER CHANGE.
   */
  static constexpr auto MAGIC = "polyipc";
  static constexpr auto MAGIC_SIZE = 7;

  static constexpr uint8_t VERSION = 0;

  union header {
    struct header_data {
      char magic[MAGIC_SIZE];
      /**
       * Version number of the message format.
       */
      uint8_t version;
      /**
       * Size of the following message in bytes
       */
      uint32_t size;
    } __attribute__((packed)) s;
    char d[sizeof(header_data)];
  };

  static constexpr size_t HEADER_SIZE = sizeof(header);
  static_assert(sizeof(header) == 12, "");
  static_assert(sizeof(header::header_data) == 12, "");
}  // namespace ipc

POLYBAR_NS_END

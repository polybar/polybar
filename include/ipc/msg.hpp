#pragma once

#include "common.hpp"

#include <array>
#include <cstdint>

POLYBAR_NS

/**
 * Defines the binary message format for IPC communications over the IPC socket.
 *
 * This is an internal API, do not connect to the socket using 3rd party programs, always use `polybar-msg`.
 */
namespace ipc {
  /**
   * Magic string prefixed to every ipc message.
   *
   * THIS MUST NEVER CHANGE.
   */
  static constexpr std::array<uint8_t, 7> MAGIC = {'p', 'o', 'l', 'y', 'i', 'p', 'c'};
  static const string MAGIC_STR = string(reinterpret_cast<const char*>(MAGIC.data()), MAGIC.size());

  static constexpr uint8_t VERSION = 0;

  using type_t = uint8_t;

  /**
   * Message type indicating success.
   */
  static constexpr type_t TYPE_OK = 0;
  static constexpr type_t TYPE_ERR = 255;

  union header {
    struct header_data {
      uint8_t magic[MAGIC.size()];
      /**
       * Version number of the message format.
       */
      uint8_t version;
      /**
       * Size of the following message in bytes
       */
      uint32_t size;
      /**
       * Type of the message that follows.
       *
       * Meaning of the values depend on version.
       * Only TYPE_OK(0) indicate success and TYPE_ERR(255) always indicates an error, in which case the entire message
       * is a string.
       */
      type_t type;
    } __attribute__((packed)) s;
    uint8_t d[sizeof(header_data)];
  };

  /**
   * Size of the standard header shared by all versions.
   *
   * THIS MUST NEVER CHANGE.
   */
  static constexpr size_t HEADER_SIZE = 13;
  static_assert(sizeof(header) == HEADER_SIZE, "");
  static_assert(sizeof(header::header_data) == HEADER_SIZE, "");

  /**
   * Definitions for version 0 of the IPC message format.
   *
   * The format is very simple. The header defines the type (cmd or action) and the payload is the message for that type
   * as a string.
   */
  namespace v0 {
    enum class ipc_type : type_t {
      /**
       * Message type for ipc commands
       */
      CMD = 1,
      /**
       * Message type for ipc module actions
       */
      ACTION = 2,
    };
  }
}  // namespace ipc

POLYBAR_NS_END

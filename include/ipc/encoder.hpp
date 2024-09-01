#pragma once

#include "common.hpp"
#include "errors.hpp"
#include "ipc/msg.hpp"

#include <cstdint>

POLYBAR_NS

namespace ipc {
  vector<uint8_t> encode(const type_t type, const vector<uint8_t>& data = {});
  vector<uint8_t> encode(const type_t type, const string& data);
}  // namespace ipc

POLYBAR_NS_END

#include "ipc/encoder.hpp"

#include <cassert>
#include <cstring>
#include <cstdint>

POLYBAR_NS

namespace ipc {
  template <typename V>
  vector<uint8_t> encode(const type_t type, const V& payload) {
    size_t total_size = HEADER_SIZE + payload.size();
    std::vector<uint8_t> data(total_size);

    auto* msg_header = reinterpret_cast<header*>(data.data());
    std::copy(MAGIC.begin(), MAGIC.end(), msg_header->s.magic);
    msg_header->s.version = VERSION;
    msg_header->s.size = payload.size();
    msg_header->s.type = type;

    std::copy(payload.begin(), payload.end(), data.begin() + HEADER_SIZE);
    return data;
  }

  vector<uint8_t> encode(const type_t type, const vector<uint8_t>& payload) {
    return encode<vector<uint8_t>>(type, payload);
  }

  vector<uint8_t> encode(const type_t type, const string& payload) {
    return encode<string>(type, payload);
  }

}  // namespace ipc
POLYBAR_NS_END

#include "ipc/encoder.hpp"

#include <cassert>
#include <cstring>

POLYBAR_NS

namespace ipc {
  template <typename V>
  vector<uint8_t> encode(const type_t type, const V& payload) {
    size_t total_size = HEADER_SIZE + payload.size();
    std::vector<uint8_t> data(total_size);

    auto* header = reinterpret_cast<ipc::header*>(data.data());
    std::copy(ipc::MAGIC.begin(), ipc::MAGIC.end(), header->s.magic);
    header->s.version = ipc::VERSION;
    header->s.size = payload.size();
    header->s.type = type;

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

#include "ipc/encoder.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "common/test.hpp"
#include "gmock/gmock.h"
#include "ipc/decoder.hpp"
#include "ipc/msg.hpp"

using namespace polybar;
using namespace ipc;
using ::testing::InSequence;

static logger null_logger(loglevel::TRACE);

class MockCallback {
 public:
  MOCK_METHOD(void, cb, (uint8_t version, type_t, const vector<uint8_t>&));
};

class EncoderTest : public ::testing::Test, public testing::WithParamInterface<string> {
 protected:
  MockCallback cb;
  decoder dec{null_logger, [this](uint8_t version, auto type, const auto& data) { cb.cb(version, type, data); }};
};

vector<string> encoder_list = {
    ""s,
    "foo"s,
    "\0\1"s,
};

INSTANTIATE_TEST_SUITE_P(Inst, EncoderTest, ::testing::ValuesIn(encoder_list));

TEST_P(EncoderTest, simple) {
  auto param = GetParam();
  const auto encoded = encode(TYPE_ERR, param);
  const header* h = reinterpret_cast<const header*>(encoded.data());

  EXPECT_EQ(std::memcmp(h->s.magic, MAGIC.data(), MAGIC.size()), 0);
  EXPECT_EQ(h->s.version, 0);
  EXPECT_EQ(h->s.size, param.size());
  EXPECT_EQ(h->s.type, TYPE_ERR);
  if (!param.empty()) {
    EXPECT_EQ(std::memcmp(encoded.data() + HEADER_SIZE, param.data(), param.size()), 0);
  }
}

TEST_P(EncoderTest, roundtrip) {
  auto param = GetParam();
  auto payload = vector<uint8_t>(param.begin(), param.end());
  const auto encoded = encode(TYPE_ERR, param);

  EXPECT_CALL(cb, cb(0, TYPE_ERR, payload)).Times(1);
  EXPECT_NO_THROW(dec.on_read(encoded.data(), encoded.size()));
}

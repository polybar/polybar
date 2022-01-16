#include "ipc/client.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "common/test.hpp"
#include "components/logger.hpp"
#include "gmock/gmock.h"
#include "ipc/msg.hpp"

using namespace polybar;
using namespace ipc;
using ::testing::InSequence;

static std::vector<uint8_t> get_msg(
    decltype(MAGIC) magic, uint32_t version, v0::ipc_type type, const std::string& msg) {
  std::vector<uint8_t> data(HEADER_SIZE);
  header* header = reinterpret_cast<ipc::header*>(data.data());
  std::copy(magic.begin(), magic.end(), header->s.magic);
  header->s.version = version;
  header->s.size = msg.size();
  header->s.type = to_integral(type);

  data.insert(data.end(), msg.begin(), msg.end());

  return data;
}

static decltype(MAGIC) MAGIC_WRONG = {'0', '0', '0', '0', '0', '0', '0'};

static auto MSG1 = get_msg(MAGIC, VERSION, v0::ipc_type::ACTION, "foobar");
static auto MSG_WRONG1 = get_msg(MAGIC_WRONG, VERSION, v0::ipc_type::ACTION, "");
static auto MSG_WRONG2 = get_msg(MAGIC, 120, v0::ipc_type::ACTION, "");
static auto MSG_WRONG3 = get_msg(MAGIC_WRONG, 120, v0::ipc_type::ACTION, "");

class MockCallback {
 public:
  MOCK_METHOD(void, cb, (uint8_t version, v0::ipc_type, const vector<uint8_t>&));
};

class IpcClientTest : public ::testing::Test {
 protected:
  MockCallback cb;
  client cl{logger::make(), [this](uint8_t version, auto type, const auto& data) { cb.cb(version, type, data); }};
};

TEST_F(IpcClientTest, single_msg) {
  EXPECT_CALL(cb, cb(0, v0::ipc_type::ACTION, vector<uint8_t>(MSG1.begin() + HEADER_SIZE, MSG1.end()))).Times(1);
  EXPECT_TRUE(cl.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong1) {
  EXPECT_FALSE(cl.on_read(MSG_WRONG1.data(), MSG_WRONG1.size()));
  // After an error, any further read fails
  EXPECT_FALSE(cl.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong2) {
  EXPECT_FALSE(cl.on_read(MSG_WRONG2.data(), MSG_WRONG2.size()));
  // After an error, any further read fails
  EXPECT_FALSE(cl.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong3) {
  EXPECT_FALSE(cl.on_read(MSG_WRONG3.data(), MSG_WRONG3.size()));
  // After an error, any further read fails
  EXPECT_FALSE(cl.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, byte_by_byte) {
  EXPECT_CALL(cb, cb(0, v0::ipc_type::ACTION, vector<uint8_t>(MSG1.begin() + HEADER_SIZE, MSG1.end()))).Times(1);
  for (const uint8_t c : MSG1) {
    EXPECT_TRUE(cl.on_read(&c, 1));
  }
}

TEST_F(IpcClientTest, multiple) {
  const static int NUM_ITER = 10;
  {
    InSequence seq;
    EXPECT_CALL(cb, cb(0, v0::ipc_type::ACTION, vector<uint8_t>(MSG1.begin() + HEADER_SIZE, MSG1.end())))
        .Times(NUM_ITER);
  }

  for (int i = 0; i < NUM_ITER; i++) {
    EXPECT_TRUE(cl.on_read(MSG1.data(), MSG1.size()));
  }
}

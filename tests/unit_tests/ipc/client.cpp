#include "ipc/client.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "common/test.hpp"
#include "components/logger.hpp"
#include "gmock/gmock.h"
#include "ipc/msg.hpp"

using namespace polybar;
using ::testing::InSequence;

static std::vector<uint8_t> get_msg(decltype(ipc::MAGIC) magic, uint32_t version, const std::string& msg) {
  std::vector<uint8_t> data(ipc::HEADER_SIZE);
  ipc::header* header = (ipc::header*)data.data();
  std::copy(magic.begin(), magic.end(), header->s.magic);
  header->s.version = version;
  header->s.size = msg.size();

  data.insert(data.end(), msg.begin(), msg.end());

  return data;
}

static decltype(ipc::MAGIC) MAGIC_WRONG = {'0', '0', '0', '0', '0', '0', '0'};

static auto MSG1 = get_msg(ipc::MAGIC, ipc::VERSION, "foobar");
static auto MSG_WRONG1 = get_msg(MAGIC_WRONG, ipc::VERSION, "");
static auto MSG_WRONG2 = get_msg(ipc::MAGIC, 120, "");
static auto MSG_WRONG3 = get_msg(MAGIC_WRONG, 120, "");

class MockCallback {
 public:
  MOCK_METHOD(void, cb, (uint8_t version, const vector<uint8_t>&));
};

class IpcClientTest : public ::testing::Test {
 protected:
  MockCallback cb;
  ipc::client client{logger::make(), [this](uint8_t version, const vector<uint8_t>& data) { cb.cb(version, data); }};
};

TEST_F(IpcClientTest, single_msg) {
  EXPECT_CALL(cb, cb(0, vector<uint8_t>(MSG1.begin() + ipc::HEADER_SIZE, MSG1.end()))).Times(1);
  EXPECT_TRUE(client.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong1) {
  EXPECT_FALSE(client.on_read(MSG_WRONG1.data(), MSG_WRONG1.size()));
  // After an error, any further read fails
  EXPECT_FALSE(client.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong2) {
  EXPECT_FALSE(client.on_read(MSG_WRONG2.data(), MSG_WRONG2.size()));
  // After an error, any further read fails
  EXPECT_FALSE(client.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, single_msg_wrong3) {
  EXPECT_FALSE(client.on_read(MSG_WRONG3.data(), MSG_WRONG3.size()));
  // After an error, any further read fails
  EXPECT_FALSE(client.on_read(MSG1.data(), MSG1.size()));
}

TEST_F(IpcClientTest, byte_by_byte) {
  EXPECT_CALL(cb, cb(0, vector<uint8_t>(MSG1.begin() + ipc::HEADER_SIZE, MSG1.end()))).Times(1);
  for (const uint8_t c : MSG1) {
    EXPECT_TRUE(client.on_read(&c, 1));
  }
}

TEST_F(IpcClientTest, multiple) {
  const static int NUM_ITER = 10;
  {
    InSequence seq;
    EXPECT_CALL(cb, cb(0, vector<uint8_t>(MSG1.begin() + ipc::HEADER_SIZE, MSG1.end()))).Times(NUM_ITER);
  }

  for (int i = 0; i < NUM_ITER; i++) {
    EXPECT_TRUE(client.on_read(MSG1.data(), MSG1.size()));
  }
}

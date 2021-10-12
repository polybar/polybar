#include "components/ipc_client.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "common/test.hpp"
#include "components/ipc_msg.hpp"
#include "components/logger.hpp"

using namespace polybar;

static std::vector<char> get_msg(const string& magic, uint32_t version, const std::string& msg) {
  assert(magic.size() == ipc::MAGIC_SIZE);
  std::vector<char> data(ipc::HEADER_SIZE);
  ipc::header* header = (ipc::header*)data.data();
  std::memcpy(header->s.magic, magic.data(), ipc::MAGIC_SIZE);
  header->s.version = version;
  header->s.size = msg.size();

  data.insert(data.end(), msg.begin(), msg.end());

  return data;
}

static auto MSG1 = get_msg(ipc::MAGIC, ipc::VERSION, "foobar");
static auto MSG_WRONG1 = get_msg("0000000", ipc::VERSION, "");
static auto MSG_WRONG2 = get_msg(ipc::MAGIC, 120, "");
static auto MSG_WRONG3 = get_msg("0000000", 120, "");

class IpcClientTest : public ::testing::Test {
 protected:
  ipc::client client{logger::make()};
};

TEST_F(IpcClientTest, single_msg) {
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
  for (const char c : MSG1) {
    EXPECT_TRUE(client.on_read(&c, 1));
  }
}

TEST_F(IpcClientTest, multiple) {
  for (int i = 0; i < 10; i++) {
    EXPECT_TRUE(client.on_read(MSG1.data(), MSG1.size()));
  }
}

#include "common/test.hpp"
#include "utils/memory.hpp"

using namespace polybar;

struct mytype {
  int x, y, z;
};

TEST(Memory, makeMallocPtr) {
  auto ptr = memory_util::make_malloc_ptr<mytype>();
  EXPECT_EQ(sizeof(mytype*), sizeof(ptr.get()));
  ptr.reset();
  EXPECT_EQ(nullptr, ptr.get());
}

TEST(Memory, countof) {
  mytype A[3]{{}, {}, {}};
  mytype B[8]{{}, {}, {}, {}, {}, {}, {}, {}};

  EXPECT_EQ(memory_util::countof(A), size_t{3});
  EXPECT_EQ(memory_util::countof(B), size_t{8});
}

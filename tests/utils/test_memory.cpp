#include <iomanip>
#include <lemonbuddy/utils/memory.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;

class test_memory : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_memory);
  CPPUNIT_TEST(test_make_malloc_ptr);
  CPPUNIT_TEST(test_countof);
  CPPUNIT_TEST_SUITE_END();

  struct mytype {
    int x, y, z;
  };

  void test_make_malloc_ptr() {
    auto ptr = memory_util::make_malloc_ptr<mytype>();
    CPPUNIT_ASSERT_EQUAL(sizeof(mytype*), sizeof(ptr.get()));
    ptr.reset();
    CPPUNIT_ASSERT(ptr.get() == nullptr);
  }

  void test_countof() {
    mytype A[3]{{}, {}, {}};
    mytype B[8]{{}, {}, {}, {}, {}, {}, {}, {}};

    CPPUNIT_ASSERT_EQUAL(size_t{3}, memory_util::countof(A));
    CPPUNIT_ASSERT_EQUAL(size_t{8}, memory_util::countof(B));
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_memory);

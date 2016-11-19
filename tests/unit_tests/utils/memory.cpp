#include "utils/memory.hpp"

struct mytype {
  int x, y, z;
};

int main() {
  using namespace polybar;

  "make_malloc_ptr"_test = [] {
    auto ptr = memory_util::make_malloc_ptr<mytype>();
    expect(sizeof(mytype*) == sizeof(ptr.get()));
    ptr.reset();
    expect(ptr.get() == nullptr);
  };

  "countof"_test = [] {
    mytype A[3]{{}, {}, {}};
    mytype B[8]{{}, {}, {}, {}, {}, {}, {}, {}};

    expect(memory_util::countof(A) == size_t{3});
    expect(memory_util::countof(B) == size_t{8});
  };
}

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <lemonbuddy/components/logger.hpp>
#include <lemonbuddy/utils/inotify.hpp>

#include "../unit_test.hpp"

#define CONFIGURE_ARGS(T, V, Args) T::configure<decltype(V)>(Args).create<decltype(V)>()
#define CONFIGURE(T, V) T::configure<decltype(V)>().create<decltype(V)>()

using namespace lemonbuddy;

class test_di : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_di);
  CPPUNIT_TEST(test_singleton);
  CPPUNIT_TEST(test_unique);
  CPPUNIT_TEST(test_instance);
  CPPUNIT_TEST_SUITE_END();

  void test_singleton() {
    // clang-format off
                      logger& instance1{CONFIGURE(logger, instance1)};
                const logger& instance2{CONFIGURE(logger, instance2)};
      std::shared_ptr<logger> instance3{CONFIGURE(logger, instance3)};
    boost::shared_ptr<logger> instance4{CONFIGURE(logger, instance4)};
    // clang-format on

    string mem_addr1{string_util::from_stream(std::stringstream() << &instance1)};
    string mem_addr2{string_util::from_stream(std::stringstream() << &instance2)};
    string mem_addr3{string_util::from_stream(std::stringstream() << instance3.get())};

    CPPUNIT_ASSERT_EQUAL(mem_addr1, mem_addr2);
    CPPUNIT_ASSERT_EQUAL(mem_addr2, mem_addr3);
    CPPUNIT_ASSERT_EQUAL(instance3.get(), instance4.get());
  }

  void test_unique() {
    unique_ptr<inotify_watch> instance1{inotify_util::make_watch("A")};
    unique_ptr<inotify_watch> instance2{inotify_util::make_watch("B")};
    shared_ptr<inotify_watch> instance3{inotify_util::make_watch("B")};
    shared_ptr<inotify_watch> instance4{inotify_util::make_watch("B")};

    CPPUNIT_ASSERT(instance1.get() != instance2.get());
    CPPUNIT_ASSERT(instance2.get() != instance3.get());
    CPPUNIT_ASSERT(instance3.get() != instance4.get());
  }

  void test_instance() {
    // TODO
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_di);

#include <lemonbuddy/components/logger.hpp>

#include "../unit_test.hpp"

using namespace lemonbuddy;

class test_logger : public unit_test {
 public:
  CPPUNIT_TEST_SUITE(test_logger);
  CPPUNIT_TEST(test_output);
  CPPUNIT_TEST_SUITE_END();

  void test_output() {
    //   signal(SIGPIPE, SIG_IGN);
    // std::string socket_path = BSPWM_SOCKET_PATH;
    // const char *env_bs = std::getenv("BSPWM_SOCKET");
    // if (env_bs != nullptr)
    //   socket_path = std::string(env_bs);
    //   thread t([&]{
    //       this_thread::yield();
    //     auto conn = socket_util::make_unix_connection(string{socket_path});
    //
    //     while (true) {
    //       if (conn->poll(POLLHUP, 0))
    //         printf("conn closed\n");
    //       if (conn->poll(POLLIN))
    //         printf("has data\n");
    //       this_thread::sleep_for(100ms);
    //     }
    //   });
    //   t.join();
    //
    auto l = logger::configure<logger>(loglevel::TRACE).create<logger>();
    l.err("error");
    l.warn("warning");
    l.info("info");
    l.trace("trace");
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(test_logger);

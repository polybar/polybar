#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "components/logger.cpp"
#include "utils/inotify.cpp"
#include "utils/string.cpp"

#define CONFIGURE_ARGS(T, V, Args) configure_##T<decltype(V)>(Args).create<decltype(V)>()
#define CONFIGURE(T, V) configure_##T<decltype(V)>().create<decltype(V)>()

int main() {
  using namespace polybar;

  "singleton"_test = [] {
    // clang-format off
                      logger& instance1{CONFIGURE(logger, instance1)};
                const logger& instance2{CONFIGURE(logger, instance2)};
      std::shared_ptr<logger> instance3{CONFIGURE(logger, instance3)};
    boost::shared_ptr<logger> instance4{CONFIGURE(logger, instance4)};
    // clang-format on

    string mem_addr1{string_util::from_stream(stringstream() << &instance1)};
    string mem_addr2{string_util::from_stream(stringstream() << &instance2)};
    string mem_addr3{string_util::from_stream(stringstream() << instance3.get())};

    expect(mem_addr1 == mem_addr2);
    expect(mem_addr2 == mem_addr3);
    expect(instance3.get() == instance4.get());
  };

  "unique"_test = [] {
    inotify_util::watch_t instance1{inotify_util::make_watch("A")};
    inotify_util::watch_t instance2{inotify_util::make_watch("B")};
    inotify_util::watch_t instance3{inotify_util::make_watch("B")};
    inotify_util::watch_t instance4{inotify_util::make_watch("B")};

    expect(instance1.get() != instance2.get());
    expect(instance2.get() != instance3.get());
    expect(instance3.get() != instance4.get());
  };

  "instance"_test = [] {
    // TODO
  };
}

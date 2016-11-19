#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "components/logger.hpp"
#include "utils/inotify.hpp"

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

    string mem_addr1{string_util::from_stream(std::stringstream() << &instance1)};
    string mem_addr2{string_util::from_stream(std::stringstream() << &instance2)};
    string mem_addr3{string_util::from_stream(std::stringstream() << instance3.get())};

    expect(mem_addr1 == mem_addr2);
    expect(mem_addr2 == mem_addr3);
    expect(instance3.get() == instance4.get());
  };

  "unique"_test = [] {
    unique_ptr<inotify_watch> instance1{inotify_util::make_watch("A")};
    unique_ptr<inotify_watch> instance2{inotify_util::make_watch("B")};
    shared_ptr<inotify_watch> instance3{inotify_util::make_watch("B")};
    shared_ptr<inotify_watch> instance4{inotify_util::make_watch("B")};

    expect(instance1.get() != instance2.get());
    expect(instance2.get() != instance3.get());
    expect(instance3.get() != instance4.get());
  };

  "instance"_test = [] {
    // TODO
  };
}

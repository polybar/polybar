#include "components/logger.hpp"

int main() {
  using namespace lemonbuddy;

  "output"_test = [] {
    auto l = logger::configure<logger>(loglevel::TRACE).create<logger>();
    l.err("error");
    l.warn("warning");
    l.info("info");
    l.trace("trace");
  };
}

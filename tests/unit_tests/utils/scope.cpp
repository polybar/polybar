#include "utils/scope.hpp"

int main() {
  using namespace lemonbuddy;

  "on_exit"_test = [] {
    auto flag = false;
    {
      expect(!flag);
      auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
      expect(!flag);
      {
        auto handler = scope_util::make_exit_handler<>([&] { flag = true; });
      }
      expect(flag);
      flag = false;
    }
    expect(flag);
  };
}

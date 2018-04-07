#include <iomanip>
#include <iostream>

#include "common/test.hpp"
#include "utils/command.hpp"
#include "utils/file.hpp"

int main() {
  using namespace polybar;

  "expand"_test = [] {
    auto cmd = command_util::make_command("echo $HOME");
    cmd->exec();
    cmd->tail([](string home) { expect(file_util::expand("~/test") == home + "/test"); });
  };
}

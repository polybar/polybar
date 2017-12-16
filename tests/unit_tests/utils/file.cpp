#include <iomanip>
#include <iostream>

#include "components/logger.cpp"
#include "utils/command.cpp"
#include "utils/concurrency.cpp"
#include "utils/env.cpp"
#include "utils/file.cpp"
#include "utils/io.cpp"
#include "utils/process.cpp"
#include "utils/string.cpp"

int main() {
  using namespace polybar;

  "expand"_test = [] {
    auto cmd = command_util::make_command("echo $HOME");
    cmd->exec();
    cmd->tail([](string home) { expect(file_util::expand("~/test") == home + "/test"); });
  };
}

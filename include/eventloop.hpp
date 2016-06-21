#pragma once

#include <map>

#include "bar.hpp"
#include "registry.hpp"
#include "exception.hpp"
#include "modules/base.hpp"
#include "services/logger.hpp"

DefineBaseException(EventLoopTerminate);
DefineBaseException(EventLoopTerminateTimeout);

class EventLoop
{
  const int STATE_STOPPED = 1;
  const int STATE_STARTED = 2;

  std::shared_ptr<Bar> bar;
  std::shared_ptr<Registry> registry;
  std::shared_ptr<Logger> logger;

  concurrency::Atomic<int> state;

  std::thread t_write;
  std::thread t_read;

  int fd_stdin = STDIN_FILENO;
  int fd_stdout = STDOUT_FILENO;
  std::string pipe_filename;

  // <tag, module_name>
  // std::map<std::string, std::string> stdin_subs;
  std::vector<std::string> stdin_subs;

  protected:
    void loop_write();
    void loop_read();

    void read_stdin();
    void write_stdout();

    bool running();

  public:
    explicit EventLoop(std::string input_pipe);

    void start();
    void stop();
    void wait();

    void cleanup(int timeout_ms = 5000);

    void add_stdin_subscriber(std::string module_name);
};

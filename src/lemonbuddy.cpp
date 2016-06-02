#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <thread>

#include "bar.hpp"
#include "config.hpp"
#include "eventloop.hpp"
#include "lemonbuddy.hpp"
#include "registry.hpp"
#include "modules/base.hpp"
#include "services/builder.hpp"

#include "utils/cli.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/macros.hpp"
#include "utils/proc.hpp"
#include "utils/string.hpp"
#include "utils/timer.hpp"
#include "utils/xlib.hpp"

/**
 * TODO: Reload config on USR1
 * TODO: Add more documentation
 * TODO: Simplify overall flow
 */

std::unique_ptr<EventLoop> eventloop;

std::mutex pid_mtx;
std::vector<pid_t> pids;

void register_pid(pid_t pid) {
  std::lock_guard<std::mutex> lck(pid_mtx);
  pids.emplace_back(pid);
}
void unregister_pid(pid_t pid) {
  std::lock_guard<std::mutex> lck(pid_mtx);
  pids.erase(std::remove(pids.begin(), pids.end(), pid), pids.end());
}
void register_command_handler(const std::string& module_name) {
  eventloop->add_stdin_subscriber(module_name);
}

/**
 * Main entry point woop!
 */
int main(int argc, char **argv)
{
  int retval = EXIT_SUCCESS;
  auto logger = get_logger();

  sigset_t pipe_mask;
  sigemptyset(&pipe_mask);
  sigaddset(&pipe_mask, SIGPIPE);
  if (pthread_sigmask(SIG_BLOCK, &pipe_mask, nullptr) == -1)
    logger->fatal(StrErrno());

  try {
    auto usage = "Usage: "+ std::string(argv[0]) + " bar_name [OPTION...]";

    cli::add_option("-h", "--help", "Show help options");
    cli::add_option("-c", "--config", "FILE", "Path to the configuration file");
    cli::add_option("-p", "--pipe", "FILE", "Path to the input pipe");
    cli::add_option("-l", "--log", "LEVEL", "Set the logging verbosity", {"info","debug","trace"});
    cli::add_option("-d", "--dump", "PARAM", "Show value of PARAM in section [bar_name]");
    cli::add_option("-x", "--print-exec", "Print the generated command line string used to start the lemonbar process");
    cli::add_option("-w", "--print-wmname", "Print the generated WM_NAME");

    /**
     * Parse command line arguments
     */
    if (argc < 2 || cli::is_option(argv[1], "-h", "--help") || argv[1][0] == '-')
      cli::usage(usage, argc > 1);
    cli::parse(2, argc, argv);
    if (cli::has_option("help"))
      cli::usage(usage);

    /**
     * Set logging verbosity
     */
    if (cli::has_option("log")) {
      if (cli::match_option_value("log", "info"))
        logger->add_level(LogLevel::LEVEL_INFO);
      else if (cli::match_option_value("log", "debug"))
        logger->add_level(LogLevel::LEVEL_INFO | LogLevel::LEVEL_DEBUG);
      else if (cli::match_option_value("log", "trace"))
        logger->add_level(LogLevel::LEVEL_INFO | LogLevel::LEVEL_DEBUG | LogLevel::LEVEL_TRACE);
    }

    /**
     * Load configuration file
     */
    const char *env_home = std::getenv("HOME");
    const char *env_xdg_config_home = std::getenv("XDG_CONFIG_HOME");

    if (cli::has_option("config")) {
      auto config_file = cli::get_option_value("config");

      if (env_home != nullptr)
        config_file = string::replace(cli::get_option_value("config"), "~", std::string(env_home));

      config::load(config_file);
    } else if (env_xdg_config_home != nullptr)
      config::load(env_xdg_config_home, "lemonbuddy/config");
    else if (env_home != nullptr)
      config::load(env_home, ".config/lemonbuddy/config");
    else
      throw ApplicationError("Could not find config file. Specify the location using --config=PATH");

    /**
     * Check if the specified bar exist
     */
    std::vector<std::string> defined_bars;
    for (auto &section : config::get_tree()) {
      if (section.first.find("bar/") == 0)
        defined_bars.emplace_back(section.first);
    }

    if (defined_bars.empty())
      logger->fatal("There are no bars defined in the config");

    auto config_path = "bar/"+ std::string(argv[1]);
    config::set_bar_path(config_path);

    if (std::find(defined_bars.begin(), defined_bars.end(), config_path) == defined_bars.end()) {
      logger->error("The bar \""+ config_path.substr(4) +"\" is not defined in the config");
      logger->info("Available bars:");
      for (auto &bar : defined_bars) logger->info("  "+ bar.substr(4));
      std::exit(EXIT_FAILURE);
    }

    if (config::get_tree().get_child_optional(config_path) == boost::none) {
      logger->fatal("Bar \""+ std::string(argv[1]) +"\" does not exist");
    }

    /**
     * Dump specified config value
     */
    if (cli::has_option("dump")) {
      std::cout << config::get<std::string>(config_path, cli::get_option_value("dump")) << std::endl;
      return EXIT_SUCCESS;
    }

    if (cli::has_option("print-exec")) {
      std::cout << get_bar()->get_exec_line() << std::endl;
      return EXIT_SUCCESS;
    }

    if (cli::has_option("print-wmname")) {
      std::cout << get_bar()->opts->wm_name << std::endl;
      return EXIT_SUCCESS;
    }

    /**
     * Set path to input pipe file
     */
    std::string pipe_file;

    if (cli::has_option("pipe")) {
      pipe_file = cli::get_option_value("pipe");
    } else {
      pipe_file = "/tmp/lemonbuddy.pipe."
        + get_bar()->opts->wm_name
        + "."
        + std::to_string(proc::get_process_id());
      auto fptr = std::make_unique<io::file::FilePtr>(pipe_file, "a+");
      if (!*fptr)
        throw ApplicationError(StrErrno());
    }

    /**
     * Create and start the main event loop
     */
    eventloop = std::make_unique<EventLoop>(pipe_file);

    eventloop->start();
    eventloop->wait();

  } catch (Exception &e) {
    logger->error(e.what());
  }

  if (eventloop)
    eventloop->stop();

  /**
   * Terminate forked sub processes
   */
  if (!pids.empty()) {
    logger->info("Terminating "+ IntToStr(pids.size()) +" spawned process"+ (pids.size() > 1 ? "es" : ""));

    for (auto &&pid : pids)
      proc::kill(pid, SIGKILL);
  }

  if (eventloop)
    eventloop->cleanup();

  while (proc::wait_for_completion_nohang() > 0);

  return retval;
}

#include <X11/Xlib-xcb.h>

#include "common.hpp"
#include "components/command_line.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/logger.hpp"
#include "config.hpp"
#include "utils/env.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "x11/xutils.hpp"

using namespace polybar;

using std::cout;
using std::endl;

struct exit_success {};
struct exit_failure {};

int main(int argc, char** argv) {
  // clang-format off
  const command_line::options opts{
    command_line::option{"-h", "--help", "Show help options"},
      command_line::option{"-v", "--version", "Print version information"},
      command_line::option{"-l", "--log", "Set the logging verbosity (default: WARNING)", "LEVEL", {"warning", "info", "trace"}},
      command_line::option{"-q", "--quiet", "Be quiet (will override -l)"},
      command_line::option{"-c", "--config", "Path to the configuration file", "FILE"},
      command_line::option{"-r", "--reload", "Reload when the configuration has been modified"},
      command_line::option{"-d", "--dump", "Show value of PARAM in section [bar_name]", "PARAM"},
      command_line::option{"-w", "--print-wmname", "Print the generated WM_NAME"},
      command_line::option{"-s", "--stdout", "Output data to stdout instead of drawing the X window"},
  };
  // clang-format on

  uint8_t exit_code{EXIT_SUCCESS};
  bool reload{false};

  logger& logger{const_cast<class logger&>(logger::make(loglevel::WARNING))};

  try {
    //==================================================
    // Connect to X server
    //==================================================
    XInitThreads();
    xcb_connection_t* xcbconn{nullptr};

    if ((xcbconn = xutils::get_connection()) == nullptr) {
      logger.err("A connection to X could not be established... ");
      throw exit_failure{};
    }

    connection{xcbconn}.preload_atoms();
    connection{xcbconn}.query_extensions();

    //==================================================
    // Block all signals by default
    //==================================================
    sigset_t blockmask;
    sigfillset(&blockmask);

    if (pthread_sigmask(SIG_BLOCK, &blockmask, nullptr) == -1) {
      throw system_error("Failed to block signals");
    }

    //==================================================
    // Parse command line arguments
    //==================================================
    string scriptname{argv[0]};
    vector<string> args(argv + 1, argv + argc);

    unique_ptr<cliparser> cli{command_line::parser::make(scriptname, opts)};
    cli->process_input(args);

    if (cli->has("quiet")) {
      logger.verbosity(loglevel::ERROR);
    } else if (cli->has("log")) {
      logger.verbosity(cli->get("log"));
    }

    if (cli->has("help")) {
      cli->usage();
      throw exit_success{};
    } else if (cli->has("version")) {
      print_build_info(version_details(args));
      throw exit_success{};
    } else if (args.empty() || args[0][0] == '-') {
      cli->usage();
      throw exit_failure{};
    }

    //==================================================
    // Load user configuration
    //==================================================
    config& conf{const_cast<config&>(config::make())};

    if (cli->has("config")) {
      conf.load(cli->get("config"), args[0]);
    } else if (env_util::has("XDG_CONFIG_HOME")) {
      conf.load(env_util::get("XDG_CONFIG_HOME") + "/polybar/config", args[0]);
    } else if (env_util::has("HOME")) {
      conf.load(env_util::get("HOME") + "/.config/polybar/config", args[0]);
    } else {
      throw application_error("Define configuration using --config=PATH");
    }

    //==================================================
    // Dump requested data
    //==================================================
    if (cli->has("dump")) {
      cout << conf.get<string>(conf.bar_section(), cli->get("dump")) << endl;
      throw exit_success{};
    }

    //==================================================
    // Create controller and run application
    //==================================================
    unique_ptr<controller> ctrl;
    bool enable_ipc{false};
    watch_t confwatch;

    if (!cli->has("print-wmname")) {
      enable_ipc = conf.get<bool>(conf.bar_section(), "enable-ipc", false);
    }
    if (!cli->has("print-wmname") && cli->has("reload")) {
      inotify_util::make_watch(conf.filepath());
    }

    ctrl = controller::make(move(confwatch), enable_ipc, cli->has("stdout"));

    if (cli->has("print-wmname")) {
      cout << ctrl->opts().wmname << endl;
      throw exit_success{};
    }

    ctrl->setup();

    if (!ctrl->run()) {
      reload = true;
    }

    //==================================================
    // Unblock signals
    //==================================================
    if (pthread_sigmask(SIG_UNBLOCK, &blockmask, nullptr) == -1) {
      throw system_error("Failed to unblock signals");
    }
  } catch (const exit_success& term) {
    exit_code = EXIT_SUCCESS;
  } catch (const exit_failure& term) {
    exit_code = EXIT_FAILURE;
  } catch (const exception& err) {
    logger.err(err.what());
    exit_code = EXIT_FAILURE;
  }

  if (!reload) {
    logger.info("Reached end of application...");
    return exit_code;
  }

  try {
    logger.info("Reload application...");
    process_util::exec(argv[0], argv);
  } catch (const system_error& err) {
    logger.err("execlp() failed (%s)", strerror(errno));
  }

  return EXIT_FAILURE;
}

#include <X11/Xlib-xcb.h>

#include "common.hpp"
#include "components/bar.hpp"
#include "components/command_line.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/ipc.hpp"
#include "components/logger.hpp"
#include "components/parser.hpp"
#include "components/renderer.hpp"
#include "config.hpp"
#include "utils/env.hpp"
#include "utils/file.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "x11/connection.hpp"
#include "x11/tray_manager.hpp"
#include "x11/xutils.hpp"

using namespace polybar;

int main(int argc, char** argv) {
  // clang-format off
  const command_line::options opts{
      command_line::option{"-h", "--help", "Show help options"},
      command_line::option{"-v", "--version", "Print version information"},
      command_line::option{"-l", "--log", "Set the logging verbosity (default: WARNING)", "LEVEL", {"error", "warning", "info", "trace"}},
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

  logger& logger{const_cast<decltype(logger)>(logger::make(loglevel::WARNING))};

  try {
    //==================================================
    // Connect to X server
    //==================================================
    XInitThreads();

    // Store the xcb connection pointer with a disconnect deleter
    shared_ptr<xcb_connection_t> xcbconn{xutils::get_connection().get(), xutils::xcb_connection_deleter{}};

    if (!xcbconn) {
      logger.err("A connection to X could not be established... ");
      return EXIT_FAILURE;
    }

    connection conn{xcbconn.get()};
    conn.preload_atoms();
    conn.query_extensions();

    //==================================================
    // Parse command line arguments
    //==================================================
    string scriptname{argv[0]};
    vector<string> args{argv + 1, argv + argc};

    cliparser::make_type cli{cliparser::make(move(scriptname), move(opts))};
    cli->process_input(args);

    if (cli->has("quiet")) {
      logger.verbosity(loglevel::ERROR);
    } else if (cli->has("log")) {
      logger.verbosity(logger::parse_verbosity(cli->get("log")));
    }

    if (cli->has("help")) {
      cli->usage();
      return EXIT_SUCCESS;
    } else if (cli->has("version")) {
      print_build_info(version_details(args));
      return EXIT_SUCCESS;
    } else if (args.empty() || args[0][0] == '-') {
      cli->usage();
      return EXIT_FAILURE;
    }

    //==================================================
    // Load user configuration
    //==================================================
    string confpath;

    if (cli->has("config")) {
      confpath = cli->get("config");
    } else if (env_util::has("XDG_CONFIG_HOME")) {
      confpath = env_util::get("XDG_CONFIG_HOME") + "/polybar/config";
    } else if (env_util::has("HOME")) {
      confpath = env_util::get("HOME") + "/.config/polybar/config";
    } else {
      throw application_error("Define configuration using --config=PATH");
    }

    config::make_type conf{config::make(move(confpath), args[0])};

    //==================================================
    // Dump requested data
    //==================================================
    if (cli->has("dump")) {
      std::cout << conf.get<string>(conf.section(), cli->get("dump")) << std::endl;
      return EXIT_SUCCESS;
    }
    if (cli->has("print-wmname")) {
      std::cout << bar::make()->settings().wmname << std::endl;
      return EXIT_SUCCESS;
    }

    //==================================================
    // Create controller and run application
    //==================================================
    unique_ptr<ipc> ipc{};
    unique_ptr<inotify_watch> config_watch{};

    if (conf.get<bool>(conf.section(), "enable-ipc", false)) {
      ipc = ipc::make();
    }
    if (cli->has("reload")) {
      config_watch = inotify_util::make_watch(conf.filepath());
    }

    auto ctrl = controller::make(move(ipc), move(config_watch));

    if (!ctrl->run(cli->has("stdout"))) {
      reload = true;
    }
  } catch (const exception& err) {
    logger.err(err.what());
    exit_code = EXIT_FAILURE;
  }

  logger.info("Waiting for spawned processes to end");
  while (process_util::notify_childprocess())
    ;

  if (reload) {
    logger.info("Re-launching application...");
    process_util::exec(move(argv[0]), move(argv));
  }

  logger.info("Reached end of application...");
  return exit_code;
}

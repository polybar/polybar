#include <X11/Xlib-xcb.h>
#include <thread>

#include "common.hpp"
#include "components/command_line.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/logger.hpp"
#include "x11/xutils.hpp"
#include "config.hpp"
#include "utils/inotify.hpp"

using namespace lemonbuddy;

int main(int argc, char** argv) {
  XInitThreads();

  logger& logger{configure_logger<decltype(logger)>(loglevel::WARNING).create<decltype(logger)>()};

  //==================================================
  // Connect to X server
  //==================================================
  xcb_connection_t* connection;

  if ((connection = xutils::get_connection()) == nullptr) {
    logger.err("A connection to X could not be established... ");
    return EXIT_FAILURE;
  }

  int xfd = xcb_get_file_descriptor(connection);

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

  stateflag terminate{false};

  while (!terminate) {
    try {
      //==================================================
      // Parse command line arguments
      //==================================================
      cliparser cli{configure_cliparser<decltype(cli)>(argv[0], opts).create<decltype(cli)>()};

      vector<string> args;
      for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
      }

      cli.process_input(args);

      if (cli.has("quiet"))
        logger.verbosity(loglevel::ERROR);
      else if (cli.has("log"))
        logger.verbosity(cli.get("log"));

      if (cli.has("help")) {
        cli.usage();
        return EXIT_SUCCESS;
      } else if (cli.has("version")) {
        print_build_info();
        return EXIT_SUCCESS;
      } else if (args.empty() || args[0][0] == '-') {
        cli.usage();
        return EXIT_FAILURE;
      }

      //==================================================
      // Load user configuration
      //==================================================
      config& conf{configure_config<decltype(conf)>().create<decltype(conf)>()};

      if (cli.has("config"))
        conf.load(cli.get("config"), args[0]);
      else if (has_env("XDG_CONFIG_HOME"))
        conf.load(read_env("XDG_CONFIG_HOME") + "/lemonbuddy/config", args[0]);
      else if (has_env("HOME"))
        conf.load(read_env("HOME") + "/.config/lemonbuddy/config", args[0]);
      else
        throw application_error("Define configuration using --config=PATH");

      //==================================================
      // Dump requested data
      //==================================================
      if (cli.has("dump")) {
        std::cout << conf.get<string>(conf.bar_section(), cli.get("dump")) << std::endl;
        return EXIT_SUCCESS;
      }

      //==================================================
      // Create config watch if we should track changes
      //==================================================
      inotify_util::watch_t watch;

      if (cli.has("reload"))
        watch = inotify_util::make_watch(conf.filepath());

      //==================================================
      // Create controller and run application
      //==================================================
      auto app = configure_controller(watch).create<unique_ptr<controller>>();

      app->bootstrap(cli.has("stdout"), cli.has("print-wmname"));

      if (cli.has("print-wmname"))
        break;

      if ((terminate = app->run()) == false)
        logger.info("Reloading application...");

    } catch (const std::exception& err) {
      logger.err(err.what());
      return EXIT_FAILURE;
    }
  };

  logger.info("Reached end of application...");

  close(xfd);

  return EXIT_SUCCESS;
}

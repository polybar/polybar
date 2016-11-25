#include <X11/Xlib-xcb.h>

#include "common.hpp"

#include "components/bar.hpp"
#include "components/command_line.hpp"
#include "components/config.hpp"
#include "components/controller.hpp"
#include "components/logger.hpp"
#include "config.hpp"
#include "utils/env.hpp"
#include "utils/inotify.hpp"
#include "x11/xutils.hpp"

using namespace polybar;

struct exit_success {};
struct exit_failure {};

int main(int argc, char** argv) {
  uint8_t exit_code{EXIT_SUCCESS};
  stateflag quit{false};

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

  logger& logger{configure_logger<decltype(logger)>(loglevel::WARNING).create<decltype(logger)>()};

  //==================================================
  // Connect to X server
  //==================================================
  XInitThreads();

  xcb_connection_t* connection{nullptr};

  if ((connection = xutils::get_connection()) == nullptr) {
    logger.err("A connection to X could not be established... ");
    throw exit_failure{};
  }

  int xfd{xcb_get_file_descriptor(connection)};

  while (!quit) {
    try {
      //==================================================
      // Parse command line arguments
      //==================================================
      vector<string> args(argv + 1, argv + argc);
      cliparser cli{configure_cliparser<decltype(cli)>(argv[0], opts).create<decltype(cli)>()};
      cli.process_input(args);

      if (cli.has("quiet")) {
        logger.verbosity(loglevel::ERROR);
      } else if (cli.has("log")) {
        logger.verbosity(cli.get("log"));
      }

      if (cli.has("help")) {
        cli.usage();
        throw exit_success{};
      } else if (cli.has("version")) {
        print_build_info(version_details(args));
        throw exit_success{};
      } else if (args.empty() || args[0][0] == '-') {
        cli.usage();
        throw exit_failure{};
      }

      //==================================================
      // Load user configuration
      //==================================================
      config& conf{configure_config<decltype(conf)>().create<decltype(conf)>()};

      if (cli.has("config")) {
        conf.load(cli.get("config"), args[0]);
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
      if (cli.has("dump")) {
        std::cout << conf.get<string>(conf.bar_section(), cli.get("dump")) << std::endl;
        throw exit_success{};
      }

      //==================================================
      // Create config watch if we should track changes
      //==================================================
      inotify_util::watch_t watch;

      if (cli.has("reload")) {
        watch = inotify_util::make_watch(conf.filepath());
      }

      //==================================================
      // Create controller
      //==================================================
      auto ctrl = configure_controller(watch).create<unique_ptr<controller>>();

      ctrl->bootstrap(cli.has("stdout"), cli.has("print-wmname"));

      if (cli.has("print-wmname")) {
        throw exit_success{};
      }

      //==================================================
      // Run application
      //==================================================
      ctrl->run();

      if (ctrl->completed()) {
        quit = true;
      } else {
        logger.info("Reloading application...");
      }
    } catch (const exit_success& term) {
      exit_code = EXIT_SUCCESS;
      quit = true;
    } catch (const exit_failure& term) {
      exit_code = EXIT_FAILURE;
      quit = true;
    } catch (const exception& err) {
      logger.err(err.what());
      exit_code = EXIT_FAILURE;
      quit = true;
    }
  }

  logger.trace("Close connection to X server");
  xcb_disconnect(connection);
  close(xfd);

  logger.info("Reached end of application...");

  return exit_code;
}

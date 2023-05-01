#include "components/bar.hpp"
#include "components/command_line.hpp"
#include "components/config.hpp"
#include "components/config_parser.hpp"
#include "components/controller.hpp"
#include "ipc/ipc.hpp"
#include "utils/env.hpp"
#include "utils/inotify.hpp"
#include "utils/process.hpp"
#include "x11/connection.hpp"

using namespace polybar;
using namespace eventloop;

int main(int argc, char** argv) {
  // clang-format off
  const command_line::options opts{
      command_line::option{"-h", "--help", "Display this help and exit"},
      command_line::option{"-v", "--version", "Display build details and exit"},
      command_line::option{"-l", "--log", "Set the logging verbosity (default: notice)", "LEVEL", {"error", "warning", "notice", "info", "trace"}},
      command_line::option{"-q", "--quiet", "Be quiet (will override -l)"},
      command_line::option{"-c", "--config", "Path to the configuration file", "FILE"},
      command_line::option{"-r", "--reload", "Reload when the configuration has been modified"},
      command_line::option{"-d", "--dump", "Print value of PARAM in bar section and exit", "PARAM"},
      command_line::option{"-m", "--list-monitors", "Print list of available monitors and exit (Removes cloned monitors)"},
      command_line::option{"-M", "--list-all-monitors", "Print list of all available monitors (Including cloned monitors) and exit"},
      command_line::option{"-w", "--print-wmname", "Print the generated WM_NAME and exit"},
      command_line::option{"-s", "--stdout", "Output data to stdout instead of drawing it to the X window"},
      command_line::option{"-p", "--png", "Save png snapshot to FILE after running for 3 seconds", "FILE"},
  };
  // clang-format on

  unsigned char exit_code{EXIT_SUCCESS};
  bool reload{false};

  logger& logger{const_cast<decltype(logger)>(logger::make(loglevel::NOTICE))};

  try {
    //==================================================
    // Parse command line arguments
    //==================================================
    string scriptname{argv[0]};
    vector<string> args{argv + 1, argv + argc};

    command_line::parser::make_type cli{command_line::parser::make(move(scriptname), move(opts))};
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
    }

    loop loop{};

    //==================================================
    // Connect to X server
    //==================================================
    auto xcb_error = 0;
    auto xcb_screen = 0;
    auto xcb_connection = xcb_connect(nullptr, &xcb_screen);

    if (xcb_connection == nullptr) {
      throw application_error("A connection to X could not be established...");
    } else if ((xcb_error = xcb_connection_has_error(xcb_connection))) {
      throw application_error("X connection error... (what: " + connection::error_str(xcb_error) + ")");
    }

    connection& conn{connection::make(xcb_connection, xcb_screen)};
    conn.ensure_event_mask(conn.root(), XCB_EVENT_MASK_PROPERTY_CHANGE);

    //==================================================
    // List available XRandR entries
    //==================================================
    if (cli->has("list-monitors") || cli->has("list-all-monitors")) {
      bool purge_clones = !cli->has("list-all-monitors");
      auto monitors = randr_util::get_monitors(conn, true, purge_clones);
      for (auto&& mon : monitors) {
        if (mon->output == XCB_NONE) {
          printf("%s: %ix%i+%i+%i (no output%s)\n", mon->name.c_str(), mon->w, mon->h, mon->x, mon->y,
              mon->primary ? ", primary" : "");
        } else {
          printf("%s: %ix%i+%i+%i%s\n", mon->name.c_str(), mon->w, mon->h, mon->x, mon->y,
              mon->primary ? " (primary)" : "");
        }
      }
      return EXIT_SUCCESS;
    }

    //==================================================
    // Load user configuration
    //==================================================
    string confpath;

    // Make sure a bar name is passed in
    if (cli->has(1)) {
      fprintf(stderr, "Unrecognized argument \"%s\"\n", cli->get(1).c_str());
      cli->usage();
      return EXIT_FAILURE;
    }

    if (cli->has("config")) {
      confpath = cli->get("config");
    } else {
      confpath = file_util::get_config_path();

      if (string_util::ends_with(confpath, "/config")) {
        logger::make().warn(
            "Naming your configuration file 'config' is deprecated, the expected name is 'config.ini'.");
      }
    }

    if (confpath.empty()) {
      throw application_error("Define configuration using --config=PATH");
    }

    string barname;
    if (cli->has(0)) {
      barname = cli->get(0);
    }

    config_parser parser{logger, move(confpath)};
    config conf = parser.parse(move(barname));

    //==================================================
    // Dump requested data
    //==================================================
    if (cli->has("dump")) {
      printf("%s\n", conf.get(conf.section(), cli->get("dump")).c_str());
      return EXIT_SUCCESS;
    }
    if (cli->has("print-wmname")) {
      printf("%s\n", bar::make(loop, conf, true)->settings().wmname.c_str());
      return EXIT_SUCCESS;
    }

    //==================================================
    // Create controller and run application
    //==================================================
    unique_ptr<ipc::ipc> ipc{};

    if (conf.get(conf.section(), "enable-ipc", false)) {
      try {
        ipc = ipc::ipc::make(loop);
      } catch (const std::exception& e) {
        ipc.reset();
        logger.err("Disabling IPC channels due to error: %s", e.what());
      }
    }

    auto ctrl = controller::make((bool)ipc, loop, conf);

    if (!ctrl->run(cli->has("stdout"), cli->get("png"), cli->has("reload"))) {
      reload = true;
    }
  } catch (const exception& err) {
    logger.err("Uncaught exception, shutting down: %s", err.what());
    exit_code = EXIT_FAILURE;
  }

  logger.info("Waiting for spawned processes to end");
  while (process_util::notify_childprocess()) {
    ;
  }

  if (reload) {
    logger.info("Re-launching application...");
    process_util::exec(move(argv[0]), move(argv));
  }

  logger.info("Reached end of application...");
  return exit_code;
}

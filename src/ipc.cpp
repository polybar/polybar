#include "modules/ipc.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>

#include "common.hpp"
#include "components/eventloop.hpp"
#include "ipc/msg.hpp"
#include "ipc/util.hpp"
#include "utils/actions.hpp"
#include "utils/file.hpp"

using namespace polybar;
using namespace std;

static const char* exec = nullptr;
static constexpr auto USAGE = "<command=(action|cmd)> <payload> [...]";
static constexpr auto USAGE_HOOK = "hook <module-name> <hook-index>";

void display(const string& msg) {
  fprintf(stdout, "%s\n", msg.c_str());
}

void error(const string& msg) {
  throw std::runtime_error(msg);
}

void uv_error(int status, const string& msg) {
  throw std::runtime_error(msg + "(" + uv_strerror(status) + ")");
}

void usage(const string& parameters) {
  fprintf(stderr, "Usage: %s [-p pid] %s\n", exec, parameters.c_str());
}

void remove_socket(const string& handle) {
  if (unlink(handle.c_str()) == -1) {
    error("Could not remove stale ipc channel: "s + strerror(errno));
  } else {
    display("Removed stale ipc channel: " + handle);
  }
}

bool validate_type(const string& type) {
  return (type == "action" || type == "cmd" || type == "hook");
}

static vector<string> get_sockets() {
  auto sockets = file_util::glob(ipc::get_glob_socket_path());

  auto new_end = std::remove_if(sockets.begin(), sockets.end(), [](const auto& path) {
    int pid = ipc::get_pid_from_socket(path);

    if (pid <= 0) {
      return false;
    }

    if (!file_util::exists("/proc/" + to_string(pid))) {
      remove_socket(path);
      return true;
    }

    return false;
  });

  sockets.erase(new_end, sockets.end());

  return sockets;
}

static void on_write(eventloop::PipeHandle& conn) {
  // TODO listen to response
  conn.read_start(
      [&](const auto& e) {
        // TODO also print PID
        printf("READ: %.*s\n", (int)e.len, e.data);
      },
      [&]() { conn.close(); },
      [&](const auto& e) {
        conn.close();
        uv_error(e.status, "There was an error while reading to polybar's response");
      });
}

static void on_connection(eventloop::PipeHandle& conn, const ipc::type_t type, const string& payload) {
  size_t total_size = ipc::HEADER_SIZE + payload.size();

  std::vector<uint8_t> data(total_size);
  auto* header = (ipc::header*)data.data();
  std::copy(ipc::MAGIC.begin(), ipc::MAGIC.end(), header->s.magic);
  header->s.version = ipc::VERSION;
  header->s.size = payload.size();
  header->s.type = type;

  memcpy(data.data() + ipc::HEADER_SIZE, payload.data(), payload.size());
  conn.write(
      data.data(), total_size, [&]() { on_write(conn); },
      [&](const auto& e) {
        conn.close();
        uv_error(e.status, "There was an error while sending the IPC message.");
      });
}

static std::pair<ipc::type_t, string> parse_message(deque<string> args) {
  // Validate args
  const string ipc_type{args.front()};
  args.pop_front();
  string ipc_payload{args.front()};
  args.pop_front();

  if (!validate_type(ipc_type)) {
    error("\"" + ipc_type + "\" is not a valid message type.");
  }

  ipc::type_t type = ipc::TYPE_ERR;

  /*
   * Check hook specific args
   *
   * The hook type is deprecated. Its contents are translated into a hook action.
   */
  if (ipc_type == "hook") {
    if (args.size() != 1) {
      usage(USAGE_HOOK);
      throw std::runtime_error("Mismatched number of arguments for hook, expected 1, got "s + to_string(args.size()));
    } else {
      if (ipc_payload.find("module/") == 0) {
        ipc_payload.erase(0, strlen("module/"));
      }

      // Hook commands use 1-indexed hooks but actions use 0-indexed ones
      int hook_index = std::stoi(args.front()) - 1;
      args.pop_front();

      type = to_integral(ipc::v0::ipc_type::ACTION);
      ipc_payload =
          actions_util::get_action_string(ipc_payload, polybar::modules::ipc_module::EVENT_HOOK, to_string(hook_index));

      fprintf(stderr,
          "Warning: Using IPC hook commands is deprecated, use the hook action on the ipc module: %s %s \"%s\"\n", exec,
          ipc_type.c_str(), ipc_payload.c_str());
    }
  }

  if (ipc_type == "action") {
    type = to_integral(ipc::v0::ipc_type::ACTION);
    /**
     * Alternatively polybar-msg action <module name> <action> <data>
     * is also accepted
     */
    if (!args.empty()) {
      string name = ipc_payload;
      string action = args.front();
      args.pop_front();
      string data{};
      if (!args.empty()) {
        data = args.front();
        args.pop_front();
      }

      ipc_payload = actions_util::get_action_string(name, action, data);
    }
  }

  if (ipc_type == "cmd") {
    type = to_integral(ipc::v0::ipc_type::CMD);
  }

  if (!args.empty()) {
    error("Too many arguments");
  }

  assert(type != ipc::TYPE_ERR);
  return {type, ipc_payload};
}

int run(int argc, char** argv) {
  exec = argv[0];
  deque<string> args{argv + 1, argv + argc};

  string socket_path;

  auto help_pos = find_if(args.begin(), args.end(), [](const string& a) { return a == "-h" || a == "--help"; });
  if (help_pos != args.end()) {
    usage(USAGE);
    return EXIT_SUCCESS;
  }

  /* If -p <pid> is passed, check if the process is running and that
   * a valid channel socket is available
   */
  if (args.size() >= 2 && args[0].compare(0, 2, "-p") == 0) {
    auto& pid_string = args[1];
    socket_path = ipc::get_socket_path(pid_string);
    if (!file_util::exists("/proc/" + pid_string)) {
      error("No process with pid " + pid_string);
    } else if (!file_util::exists(socket_path)) {
      error("No channel available for pid " + pid_string);
    }

    args.pop_front();
    args.pop_front();
  }

  // If no pid was given, search for all open sockets.
  auto sockets = socket_path.empty() ? get_sockets() : vector<string>{socket_path};

  // Get availble channel sockets
  if (sockets.empty()) {
    error("No active ipc channels");
  }

  if (args.size() < 2) {
    usage(USAGE);
    return EXIT_FAILURE;
  }

  string payload;
  ipc::type_t type;
  std::tie(type, payload) = parse_message(args);

  bool success = true;

  eventloop::eventloop loop;

  for (auto&& channel : sockets) {
    auto& conn = loop.handle<eventloop::PipeHandle>();

    conn.connect(
        channel, [&, payload]() { on_connection(conn, type, payload); },
        [&](const auto& e) {
          fprintf(stderr, "%s: Failed to connect to '%s' (err: '%s')\n", exec, channel.c_str(), uv_strerror(e.status));
          success = false;
        });
  }

  try {
    loop.run();
  } catch (const exception& e) {
    throw std::runtime_error("Uncaught exception in eventloop: "s + e.what());
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char** argv) {
  exec = argv[0];
  try {
    return run(argc, argv);
  } catch (const std::exception& e) {
    fprintf(stderr, "%s: %s\n", exec, e.what());
    return EXIT_FAILURE;
  }
}

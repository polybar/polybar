#include "modules/ipc.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
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

static constexpr int E_NO_CHANNELS{2};
static constexpr int E_MESSAGE_TYPE{3};
static constexpr int E_INVALID_PID{4};
static constexpr int E_INVALID_CHANNEL{5};
// static constexpr int E_WRITE{6};
static constexpr int E_EXCEPTION{7};

static const char* exec = nullptr;

void display(const string& msg) {
  fprintf(stdout, "%s\n", msg.c_str());
}

void error(int exit_code, const string& msg) {
  fprintf(stderr, "%s: %s\n", exec, msg.c_str());
  exit(exit_code);
}

void usage(const string& parameters) {
  fprintf(stderr, "Usage: polybar-msg [-p pid] %s\n", parameters.c_str());
  exit(127);
}

void remove_socket(const string& handle) {
  if (unlink(handle.c_str()) == -1) {
    error(1, "Could not remove stale ipc channel: "s + strerror(errno));
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
  conn.read_start([&](const auto& e) { printf("READ: %.*s\n", (int)e.len, e.data); },
      [&]() {
        // TODO handle EOF
      },
      [&](const auto& e) {
        // TODO handle error
      });
}

static void on_connection(eventloop::PipeHandle& conn, const string& payload) {
  size_t total_size = ipc::HEADER_SIZE + payload.size();

  auto data = std::make_unique<uint8_t[]>(total_size);
  ipc::header* header = (ipc::header*)data.get();
  memcpy(header->s.magic, ipc::MAGIC, ipc::MAGIC_SIZE);
  header->s.version = ipc::VERSION;
  header->s.size = payload.size();

  memcpy(data.get() + ipc::HEADER_SIZE, payload.data(), payload.size());
  conn.write(
      data.get(), total_size, [&]() { on_write(conn); },
      [&](const auto& e) {
        // TODO handle error
        UV((int), e.status);
      });
}

int main(int argc, char** argv) {
  exec = argv[0];
  deque<string> args{argv + 1, argv + argc};

  string socket_path;

  auto help =
      find_if(args.begin(), args.end(), [](const string& a) { return a == "-h" || a == "--help"; }) != args.end();
  if (help || args.size() < 2) {
    usage("<command=(action|cmd|hook)> <payload> [...]");
  } else if (!validate_type(args[0])) {
    error(E_MESSAGE_TYPE, "\"" + args[0] + "\" is not a valid type.");
  }

  /* If -p <pid> is passed, check if the process is running and that
   * a valid channel socket is available
   */
  if (args.size() >= 2 && args[0].compare(0, 2, "-p") == 0) {
    auto& pid_string = args[0];
    socket_path = ipc::get_socket_path(pid_string);
    if (!file_util::exists("/proc/" + pid_string)) {
      error(E_INVALID_PID, "No process with pid " + pid_string);
    } else if (!file_util::exists(socket_path)) {
      error(E_INVALID_CHANNEL, "No channel available for pid " + pid_string);
    }

    args.pop_front();
    args.pop_front();
  }

  // If no pid was given, search for all open sockets.
  auto sockets = socket_path.empty() ? get_sockets() : vector<string>{socket_path};

  // Get availble channel sockets
  if (sockets.empty()) {
    error(E_NO_CHANNELS, "No active ipc channels");
  }

  // Validate args
  string ipc_type{args.front()};
  args.pop_front();
  string ipc_payload{args.front()};
  args.pop_front();

  /*
   * Check hook specific args
   *
   * The hook type is deprecated. Its contents are translated into a hook action.
   */
  if (ipc_type == "hook") {
    if (args.size() != 1) {
      usage("hook <module-name> <hook-index>");
    } else {
      if (ipc_payload.find("module/") == 0) {
        ipc_payload.erase(0, strlen("module/"));
      }

      // Hook commands use 1-indexed hooks but actions use 0-indexed ones
      int hook_index = std::stoi(args.front()) - 1;
      args.pop_front();

      ipc_type = "action";
      ipc_payload =
          actions_util::get_action_string(ipc_payload, polybar::modules::ipc_module::EVENT_HOOK, to_string(hook_index));

      fprintf(stderr,
          "Warning: Using IPC hook commands is deprecated, use the hook action on the ipc module: %s %s \"%s\"\n", exec,
          ipc_type.c_str(), ipc_payload.c_str());
    }
  }

  if (ipc_type == "action") {
    /**
     * Alternatively polybar-msg action <module name> <action> <data>
     * is also accepted
     */
    if (!args.empty()) {
      string name = ipc_payload;
      string action = args.front();
      args.pop_front();
      string data = "";
      if (!args.empty()) {
        data = args.front();
        args.pop_front();
      }

      ipc_payload = actions_util::get_action_string(name, action, data);
    }
  }

  if (!args.empty()) {
    error(1, "Too many arguments");
  }

  int exit_status = 127;

  eventloop::eventloop loop;

  for (auto&& channel : sockets) {
    try {
      auto& conn = loop.handle<eventloop::PipeHandle>();

      string payload{ipc_type + ':' + ipc_payload};

      conn.connect(
          channel,
          [&]() {
            on_connection(conn, payload);
            exit_status = 0;
          },
          [&](const auto& e) {
            fprintf(stderr, "Failed to connect to '%s' (err: '%s')\n", channel.c_str(), uv_strerror(e.status));
          });
    } catch (const exception& err) {
      error(E_EXCEPTION, err.what());
    }
  }

  try {
    loop.run();
  } catch (const exception& e) {
    fprintf(stderr, "Uncaught exception in eventloop: %s", e.what());
    return EXIT_FAILURE;
  }

  return exit_status;
}

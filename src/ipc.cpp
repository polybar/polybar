#include <fcntl.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "common.hpp"
#include "utils/file.hpp"
#include "utils/io.hpp"

using namespace polybar;
using namespace std;

#ifndef IPC_CHANNEL_PREFIX
#define IPC_CHANNEL_PREFIX "/tmp/polybar_mqueue."
#endif

void display(const string& msg) {
  fprintf(stdout, "%s\n", msg.c_str());
}

void log(int exit_code, const string& msg) {
  fprintf(stderr, "polybar-msg: %s\n", msg.c_str());
  exit(exit_code);
}

void usage(const string& parameters) {
  fprintf(stderr, "Usage: polybar-msg [-p pid] %s\n", parameters.c_str());
  exit(127);
}

void remove_pipe(const string& handle) {
  if (unlink(handle.c_str()) == -1) {
    log(1, "Could not remove stale ipc channel: "s + strerror(errno));
  } else {
    display("Removed stale ipc channel: " + handle);
  }
}

bool validate_type(const string& type) {
  return (type == "action" || type == "cmd" || type == "hook");
}

int main(int argc, char** argv) {
  const int E_NO_CHANNELS{2};
  const int E_MESSAGE_TYPE{3};
  const int E_INVALID_PID{4};
  const int E_INVALID_CHANNEL{5};
  const int E_WRITE{6};

  vector<string> args{argv + 1, argv + argc};
  string::size_type p;
  int pid{0};

  // If -p <pid> is passed, check if the process is running and that
  // a valid channel pipe is available
  if (args.size() >= 2 && args[0].compare(0, 2, "-p") == 0) {
    if (!file_util::exists("/proc/" + args[1])) {
      log(E_INVALID_PID, "No process with pid " + args[1]);
    } else if (!file_util::exists(IPC_CHANNEL_PREFIX + args[1])) {
      log(E_INVALID_CHANNEL, "No channel available for pid " + args[1]);
    }

    pid = strtol(args[1].c_str(), nullptr, 10);
    args.erase(args.begin());
    args.erase(args.begin());
  }

  // Validate args
  auto help = find_if(args.begin(), args.end(), [](string a) { return a == "-h" || a == "--help"; }) != args.end();
  if (help || args.size() < 2) {
    usage("<command=(action|cmd|hook)> <payload> [...]");
  } else if (!validate_type(args[0])) {
    log(E_MESSAGE_TYPE, "\"" + args[0] + "\" is not a valid type.");
  }

  string ipc_type{args[0]};
  args.erase(args.begin());
  string ipc_payload{args[0]};
  args.erase(args.begin());

  // Check hook specific args
  if (ipc_type == "hook") {
    if (args.size() != 1) {
      usage("hook <module-name> <hook-index>");
    } else if ((p = ipc_payload.find("module/")) != 0) {
      ipc_payload = "module/" + ipc_payload + args[0];
      args.erase(args.begin());
    } else {
      ipc_payload += args[0];
      args.erase(args.begin());
    }
  }

  // Get availble channel pipes
  auto pipes = file_util::glob(IPC_CHANNEL_PREFIX + "*"s);

  // Remove stale channel files without a running parent process
  for (auto it = pipes.rbegin(); it != pipes.rend(); it++) {
    if ((p = it->rfind('.')) == string::npos) {
      continue;
    } else if (!file_util::exists("/proc/" + it->substr(p + 1))) {
      remove_pipe(*it);
      pipes.erase(remove(pipes.begin(), pipes.end(), *it), pipes.end());
    } else if (pid && to_string(pid) != it->substr(p + 1)) {
      pipes.erase(remove(pipes.begin(), pipes.end(), *it), pipes.end());
    }
  }

  if (pipes.empty()) {
    log(E_NO_CHANNELS, "No active ipc channels");
  }

  int exit_status = 127;

  // Write message to each available channel or match
  // against pid if one was defined
  for (auto&& channel : pipes) {
    try {
      file_descriptor fd(channel, O_WRONLY | O_NONBLOCK);
      string payload{ipc_type + ':' + ipc_payload};
      if (write(fd, payload.c_str(), payload.size()) != -1) {
        display("Successfully wrote \"" + payload + "\" to \"" + channel + "\"");
        exit_status = 0;
      } else {
        log(E_WRITE, "Failed to write \"" + payload + "\" to \"" + channel + "\" (err: " + strerror(errno) + ")");
      }
    } catch (const exception& err) {
      remove_pipe(channel);
    }
  }

  return exit_status;
}
